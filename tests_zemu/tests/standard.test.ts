/** ******************************************************************************
 *  (c) 2018 - 2023 Zondax AG
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************* */

import Zemu, { zondaxMainmenuNavigation, ButtonKind } from '@zondax/zemu'
// @ts-ignore
import {OasisApp} from "@zondax/ledger-oasis";
import { models, defaultOptions, STANDARD_BLOBS } from "./common";

const ed25519 = require("ed25519-supercop");
const sha512 = require("js-sha512");

jest.setTimeout(60000)

// Derivation path. First 3 items are automatically hardened!
const path = "m/44'/474'/5'/0'/3'";


describe('Standard', function () {
  test.concurrent.each(models)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)('main menu', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const nav = zondaxMainmenuNavigation(m.name, [1, 0, 0, 4, -5])
      await sim.navigateAndCompareSnapshots('.', `${m.prefix.toLowerCase()}-mainmenu`, nav.schedule)
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)('get app version', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});
      const app = new OasisApp(sim.getTransport());
      const resp = await app.getVersion();

      console.log(resp);

      expect(resp.return_code).toEqual(0x9000);
      expect(resp.error_message).toEqual("No errors");
      expect(resp).toHaveProperty("test_mode");
      expect(resp).toHaveProperty("major");
      expect(resp).toHaveProperty("minor");
      expect(resp).toHaveProperty("patch");
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)('get address', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});
      const app = new OasisApp(sim.getTransport());

      const resp = await app.getAddressAndPubKey(path);

      console.log(resp)

      expect(resp.return_code).toEqual(0x9000);
      expect(resp.error_message).toEqual("No errors");

      const expected_bech32_address = "oasis1qphdkldpttpsj2j3l9sde9h26cwpfwqwwuhvruyu";
      const expected_pk = "aba52c0dcb80c2fe96ed4c3741af40c573a0500c0d73acda22795c37cb0f1739";

      expect(resp.bech32_address).toEqual(expected_bech32_address);
      expect(resp.pk.toString('hex')).toEqual(expected_pk);

    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)('show address', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({
        ...defaultOptions,
        model: m.name,
        approveKeyword: m.name === 'stax' ? 'QR' : '',
        approveAction: ButtonKind.ApproveTapButton,
      })
      const app = new OasisApp(sim.getTransport());

      const respRequest = app.showAddressAndPubKey(path);

      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot(), 20000);
      await sim.compareSnapshotsAndApprove(".", `${m.prefix.toLowerCase()}-show_address`);

      const resp = await respRequest;
      console.log(resp);

      expect(resp.return_code).toEqual(0x9000);
      expect(resp.error_message).toEqual("No errors");

      const expected_bech32_address = "oasis1qphdkldpttpsj2j3l9sde9h26cwpfwqwwuhvruyu";
      const expected_pk = "aba52c0dcb80c2fe96ed4c3741af40c573a0500c0d73acda22795c37cb0f1739";

      expect(resp.bech32_address).toEqual(expected_bech32_address);
      expect(resp.pk.toString('hex')).toEqual(expected_pk);
    } finally {
      await sim.close();
    }
  });

  describe.each(STANDARD_BLOBS)('Standard', function (data) {
    test.concurrent.each(models)(`Test: ${data.name}`, async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({...defaultOptions, model: m.name,});
        const app = new OasisApp(sim.getTransport());

        const pkResponse = await app.getAddressAndPubKey(path);
        expect(pkResponse.return_code).toEqual(0x9000);
        expect(pkResponse.error_message).toEqual("No errors");

        const signatureRequest = app.sign(path, data.context, data.txBlob);
        await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot(), 20000);
        await sim.compareSnapshotsAndApprove(".", `${m.prefix.toLowerCase()}-${data.name}`);

        let resp = await signatureRequest;
        console.log(resp);

        expect(resp.return_code).toEqual(0x9000);
        expect(resp.error_message).toEqual("No errors");

        const hasher = sha512.sha512_256.update(data.context)
        hasher.update(data.txBlob);
        const msgHash = Buffer.from(hasher.hex(), "hex")

        // Now verify the signature
        const valid = ed25519.verify(resp.signature, msgHash, pkResponse.pk);
        expect(valid).toEqual(true);
      } finally {
        sim.dumpEvents()
        await sim.close()
      }
    })
  })

  test.concurrent.each(models)('sign basic - invalid', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});
      const app = new OasisApp(sim.getTransport());

      const context = "oasis-core/consensus: tx for chain testing";
      let invalidMessage = Buffer.from(
        "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omd4ZmVyX3RvWCBkNhaFWEyIEubmS3EVtRLTanD3U+vDV5fke4Obyq83CWt4ZmVyX3Rva2Vuc0Blbm9uY2UAZm1ldGhvZHBzdGFraW5nLlRyYW5zZmVy",
        "base64",
      );
      invalidMessage = Buffer.concat([invalidMessage, Buffer.from("1")]);

      const pkResponse = await app.getAddressAndPubKey(path);
      console.log(pkResponse);
      expect(pkResponse.return_code).toEqual(0x9000);
      expect(pkResponse.error_message).toEqual("No errors");

      // do not wait here..
      const responseSign = await app.sign(path, context, invalidMessage);
      console.log(responseSign);

      expect(responseSign.return_code).toEqual(0x6984);
      expect(responseSign.error_message).toEqual("Data is invalid : Unexpected field");
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)('sign entity metadata - long name', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});

      const app = new OasisApp(sim.getTransport());

      const context = "oasis-metadata-registry: entity";

      const txBlob = Buffer.from(
        "a76176016375726c7568747470733a2f2f6d792e656e746974792f75726c646e616d6578335468697320697320736f6d6520746f6f6f6f6f6f6f6f6f6f6f6f6f6f6f206c6f6e6720656e74697479206e616d65202835312965656d61696c6d6d7940656e746974792e6f72676673657269616c01676b657962617365716d795f6b6579626173655f68616e646c656774776974746572716d795f747769747465725f68616e646c65",
        "hex",
      );

      const pkResponse = await app.getAddressAndPubKey(path);
      console.log(pkResponse);
      expect(pkResponse.return_code).toEqual(0x9000);
      expect(pkResponse.error_message).toEqual("No errors");

      // do not wait here..
      const signatureRequest = app.sign(path, context, txBlob);

      let resp = await signatureRequest;
      console.log(resp);

      expect(resp.return_code).toEqual(0x6984);
      expect(resp.error_message).toEqual("Data is invalid : Invalid name length (max 50 characters)");
    } finally {
      await sim.close();
    }
  });
});

describe('Issue #68', function () {

  test.concurrent.each(models)('should sign a transaction two time in a row (issue #68)', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});
      const app = new OasisApp(sim.getTransport());

      const context = "oasis-core/consensus: tx for chain testing";
      const txBlob = Buffer.from(
        "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omJ0b1UAxzzAAUY0NJFbo/OXUb63wJBbRetmYW1vdW50QGVub25jZQBmbWV0aG9kcHN0YWtpbmcuVHJhbnNmZXI=",
        "base64",
      );

      const pkResponse = await app.getAddressAndPubKey(path);
      console.log(pkResponse);
      expect(pkResponse.return_code).toEqual(0x9000);
      expect(pkResponse.error_message).toEqual("No errors");

      // do not wait here..
      const signatureRequest = app.sign(path, context, txBlob);

      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot(), 20000);
      await sim.compareSnapshotsAndApprove(".", `${m.prefix.toLowerCase()}-sign_basic`);

      let resp = await signatureRequest;
      console.log(resp);

      expect(resp.return_code).toEqual(0x9000);
      expect(resp.error_message).toEqual("No errors");

      // Need to wait a bit before signing again.
      await sim.deleteEvents();
      await Zemu.sleep(250);

      // Here we go again
      const signatureRequestBis = app.sign(path, context, txBlob);
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot(), 20000);
      await sim.compareSnapshotsAndApprove(".", `${m.prefix.toLowerCase()}-sign_basic`);

      let respBis = await signatureRequestBis;
      console.log(respBis);

      expect(respBis.return_code).toEqual(0x9000);
      expect(respBis.error_message).toEqual("No errors");

    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)('sign basic - mainnet', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});
      const app = new OasisApp(sim.getTransport());

      const context = "oasis-core/consensus: tx for chain 53852332637bacb61b91b6411ab4095168ba02a50be4c3f82448438826f23898";
      const txBlob = Buffer.from(
        "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omtiZW5lZmljaWFyeVUA8PesI5mFWUkMVHwStQ6Fieb4bsFtYW1vdW50X2NoYW5nZUBlbm9uY2UBZm1ldGhvZG1zdGFraW5nLkFsbG93",
        "base64",
      );

      const pkResponse = await app.getAddressAndPubKey(path);
      console.log(pkResponse);
      expect(pkResponse.return_code).toEqual(0x9000);
      expect(pkResponse.error_message).toEqual("No errors");

      // do not wait here..
      const signatureRequest = app.sign(path, context, txBlob);

      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot(), 20000);
      await sim.compareSnapshotsAndApprove(".", `${m.prefix.toLowerCase()}-sign_basic_allow_mainnet`);

      let resp = await signatureRequest;
      console.log(resp);

      expect(resp.return_code).toEqual(0x9000);
      expect(resp.error_message).toEqual("No errors");

      const hasher = sha512.sha512_256.update(context)
      hasher.update(txBlob);
      const msgHash = Buffer.from(hasher.hex(), "hex")

      // Now verify the signature
      const valid = ed25519.verify(resp.signature, msgHash, pkResponse.pk);
      expect(valid).toEqual(true);
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)('cast vote - yes mainnet', async function (m) {
    const sim = new Zemu(m.path);
    try {
      await sim.start({...defaultOptions, model: m.name,});
      const app = new OasisApp(sim.getTransport());

      const context = "oasis-core/consensus: tx for chain 53852332637bacb61b91b6411ab4095168ba02a50be4c3f82448438826f23898";
      const txBlob = Buffer.from(
        "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omJpZBoAmJaAZHZvdGUBZW5vbmNlAWZtZXRob2RzZ292ZXJuYW5jZS5DYXN0Vm90ZQ==",
        "base64",
      );

      const pkResponse = await app.getAddressAndPubKey(path);
      console.log(pkResponse);
      expect(pkResponse.return_code).toEqual(0x9000);
      expect(pkResponse.error_message).toEqual("No errors");

      // do not wait here..
      const signatureRequest = app.sign(path, context, txBlob);

      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot(), 20000);
      await sim.compareSnapshotsAndApprove(".", `${m.prefix.toLowerCase()}-cast_vote_yes_mainnet`);

      let resp = await signatureRequest;
      console.log(resp);

      expect(resp.return_code).toEqual(0x9000);
      expect(resp.error_message).toEqual("No errors");

      const hasher = sha512.sha512_256.update(context)
      hasher.update(txBlob);
      const msgHash = Buffer.from(hasher.hex(), "hex")

      // Now verify the signature
      const valid = ed25519.verify(resp.signature, msgHash, pkResponse.pk);
      expect(valid).toEqual(true);
    } finally {
      await sim.close();
    }
  });
})

it('hash', async function () {
  const txBlob = Buffer.from(
    "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omd4ZmVyX3RvWCBkNhaFWEyIEubmS3EVtRLTanD3U+vDV5fke4Obyq83CWt4ZmVyX3Rva2Vuc0Blbm9uY2UAZm1ldGhvZHBzdGFraW5nLlRyYW5zZmVy",
    "base64",
  );
  const context = "oasis-core/consensus: tx for chain testing";
  const hasher = sha512.sha512_256.update(context)
  hasher.update(txBlob);
  const hash = Buffer.from(hasher.hex(), "hex")
  console.log(hash.toString("hex"))
  expect(hash.toString("hex")).toEqual("86f53ebf15a09c4cd1cf7a52b8b381d74a2142996aca20690d2e750c1d262ec0")
});
