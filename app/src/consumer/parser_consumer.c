/*******************************************************************************
*   (c) 2019 Zondax GmbH
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
********************************************************************************/
#if defined(APP_CONSUMER)

#include <stdio.h>
#include <zxmacros.h>
#include <zxerror.h>
#include <zxformat.h>
#include <bech32.h>
#include <base64.h>
#include "parser_impl_con.h"
#include "bignum.h"
#include "parser.h"
#include "parser_txdef_con.h"
#include "coin.h"
#include "sha512.h"

const char addressV0_Secp256k1_ethContext[40] = "oasis-runtime-sdk/address: secp256k1eth";

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
// For some reason NanoX requires this function
void __assert_fail(__Z_UNUSED const char * assertion, __Z_UNUSED const char * file, __Z_UNUSED unsigned int line, __Z_UNUSED const char * function){
    while(1) {};
}
#endif

static const pt_lookup_t paraTime_lookup_helper[] = {
    {CIPHER_MAIN_RUNID, CIPHER_MAIN_TO_ADDR, 9, "Cipher"},
    {CIPHER_TEST_RUNID, CIPHER_TEST_TO_ADDR, 9, "Cipher"},
    {EMERALD_MAIN_RUNID, EMERALD_MAIN_TO_ADDR, 18, "Emerald"},
    {EMERALD_TEST_RUNID, EMERALD_TEST_TO_ADDR, 18, "Emerald"},
    {SAPPHIRE_MAIN_RUNID, SAPPHIRE_MAIN_TO_ADDR,18, "Sapphire"},
    {SAPPHIRE_TEST_RUNID, SAPPHIRE_TEST_TO_ADDR, 18, "Sapphire"},
};

static const char * methodsMap[] = {
    "unkown", "Transfer", "Burn", "Withdraw", "Allow", "Add escrow",
    "Reclaim escrow", "Amend commission schedule", "Deregister Entity",
    "Unfreeze Node", "Register Entity", "Submit proposal", "Cast vote",
    "Transfer (Paratime)", "Deposit (Paratime)", "Withdraw (Paratime)"
};

parser_error_t parser_parse(parser_context_t *ctx, const uint8_t *data, size_t dataLen) {
    CHECK_PARSER_ERR(parser_init(ctx, data, dataLen))
    CHECK_PARSER_ERR(_readContext(ctx, &parser_tx_obj))
    CHECK_PARSER_ERR(_extractContextSuffix(&parser_tx_obj))

    // Read after we determine context
    CHECK_PARSER_ERR(_read(ctx, &parser_tx_obj));

    return parser_ok;
}

parser_error_t parser_validate(const parser_context_t *ctx) {
    CHECK_PARSER_ERR(_validateTx(ctx, &parser_tx_obj))

    // Iterate through all items to check that all can be shown and are valid
    uint8_t numItems = 0;
    CHECK_PARSER_ERR(parser_getNumItems(ctx, &numItems))

    char tmpKey[40];
    char tmpVal[40];

    for (uint8_t idx = 0; idx < numItems; idx++) {
        uint8_t pageCount = 0;
        CHECK_PARSER_ERR(parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount))
        CHECK_APP_CANARY()
    }

    return parser_ok;
}

parser_error_t parser_getNumItems(const parser_context_t *ctx, uint8_t *num_items) {
    *num_items = _getNumItems(ctx, &parser_tx_obj);
    if (parser_tx_obj.context.suffixLen > 0) {
        (*num_items)++;
    }
    return parser_ok;
}

__Z_INLINE parser_error_t parser_getType(__Z_UNUSED const parser_context_t *ctx, char *outVal, uint16_t outValLen) {

    char * str= {0};
    switch(parser_tx_obj.type) {
        case txType :
            if (parser_tx_obj.oasis.tx.method < stakingTransfer ||
                parser_tx_obj.oasis.tx.method > governanceCastVote) {
                return parser_unexpected_method;
            }

            str = (char *)PIC(methodsMap[parser_tx_obj.oasis.tx.method]);
#if defined(TARGET_NANOS)
            if (parser_tx_obj.oasis.tx.method == stakingAmendCommissionSchedule) {
                snprintf(outVal, outValLen, "Amend commission  schedule");
                return parser_ok;
            }
#endif
            snprintf(outVal, outValLen, "%s", str);
            return parser_ok;
        case runtimeType:
            if (parser_tx_obj.oasis.runtime.call.method < accountsTransfer ||
                parser_tx_obj.oasis.runtime.call.method > consensusWithdraw) {
                return parser_unexpected_method;
            }

            str = (char *)PIC(methodsMap[parser_tx_obj.oasis.runtime.call.method]);
            snprintf(outVal, outValLen, "%s", str);
            return parser_ok;
        default:
            break;
    }

    return parser_unexpected_method;
}

#define LESS_THAN_64_DIGIT(num_digit) if (num_digit > 64) return parser_value_out_of_range;

__Z_INLINE bool format_quantity(const quantity_t *q,
                                uint8_t *bcd, uint16_t bcdSize,
                                char *bignum, uint16_t bignumSize) {

    bignumBigEndian_to_bcd(bcd, bcdSize, q->buffer, q->len);
    return bignumBigEndian_bcdprint(bignum, bignumSize, bcd, bcdSize);
}

__Z_INLINE parser_error_t parser_printQuantity(const quantity_t *q, const context_t *net,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
    // upperbound 2**(64*8)
    // results in 155 decimal digits => max 78 bcd bytes

    // Too many digits, we cannot format this
    LESS_THAN_64_DIGIT(q->len)

    const char* denom = "";
    const uint8_t hashSize = sizeof(MAINNET_GENESIS_HASH) - 1;
    if (hashSize == net->suffixLen) {
        if (MEMCMP((const char *) net->suffixPtr, MAINNET_GENESIS_HASH, hashSize) == 0) {
            denom = COIN_MAINNET_DENOM;
        } else if (MEMCMP((const char *) net->suffixPtr, TESTNET_GENESIS_HASH, hashSize) == 0) {
            denom = COIN_TESTNET_DENOM;
        }
    }

    snprintf(outVal, outValLen, "%s ", denom);
    outVal += strlen(denom) + 1;
    outValLen -= strlen(denom) + 1;

    char bignum[160] = {0};
    union {
        // overlapping arrays to avoid excessive stack usage. Do not use at the same time
        uint8_t bcd[80];
        char output[160];
    } overlapped;

    MEMZERO(&overlapped, sizeof(overlapped));

    if (!format_quantity(q, overlapped.bcd, sizeof(overlapped.bcd), bignum, sizeof(bignum))) {
        return parser_unexpected_value;
    }

    fpstr_to_str(overlapped.output, sizeof(overlapped.output), bignum, COIN_AMOUNT_DECIMAL_PLACES);
    number_inplace_trimming(overlapped.output, 1);
    pageString(outVal, outValLen, overlapped.output, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printRuntimeQuantity(const meta_t *meta, const quantity_t *q, const string_t *denomination, char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
    // upperbound 2**(64*8)
    // results in 155 decimal digits => max 78 bcd bytes

    // Too many digits, we cannot format this
    LESS_THAN_64_DIGIT(q->len)
    const char* denom = "";
    uint8_t decimal = 0;
    //empty denomination
    for (size_t i = 0; i < array_length(paraTime_lookup_helper); i++) {
        if (denomination->len == 0) {
            if (MEMCMP(meta->chain_context, MAINNET_GENESIS_HASH, HASH_SIZE) == 0 && 
                MEMCMP(meta->runtime_id,  PIC(paraTime_lookup_helper[i].runid), HASH_SIZE) == 0) {
                denom = COIN_MAINNET_DENOM;
                decimal = paraTime_lookup_helper[i].decimals;
                break;
            } else if (MEMCMP(meta->chain_context, TESTNET_GENESIS_HASH, HASH_SIZE) == 0 &&
                MEMCMP(meta->runtime_id,  PIC(paraTime_lookup_helper[i].runid), HASH_SIZE) == 0) {
                denom = COIN_TESTNET_DENOM;
                decimal = paraTime_lookup_helper[i].decimals;
                break;
            }
        } else {
            if (MEMCMP(meta->runtime_id,  PIC(paraTime_lookup_helper[i].runid), HASH_SIZE) == 0) {
                denom = " ";
                decimal = paraTime_lookup_helper[i].decimals;
                break;
            }
        }
    }

    snprintf(outVal, outValLen, "%s ", denom);
    outVal += strlen(denom) + 1;
    outValLen -= strlen(denom) + 1;

    char bignum[160] = {0};
    union {
        // overlapping arrays to avoid excessive stack usage. Do not use at the same time
        uint8_t bcd[80];
        char output[160];
    } overlapped;

    MEMZERO(&overlapped, sizeof(overlapped));

    if (!format_quantity(q, overlapped.bcd, sizeof(overlapped.bcd), bignum, sizeof(bignum))) {
        return parser_unexpected_value;
    }

    fpstr_to_str(overlapped.output, sizeof(overlapped.output), bignum, decimal);
    number_inplace_trimming(overlapped.output, 1);
    pageString(outVal, outValLen, overlapped.output, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printQuantityWithSign(const quantity_t *q, const context_t *net,
                                                        bool is_negative, char *outVal,
                                                        uint16_t outValLen, uint8_t pageIdx,
                                                        uint8_t *pageCount) {
    // upperbound 2**(64*8)
    // results in 155 decimal digits => max 78 bcd bytes

    // Too many digits, we cannot format this
    LESS_THAN_64_DIGIT(q->len)

    const char* denom = "";
    const uint8_t hashSize = sizeof(MAINNET_GENESIS_HASH) - 1;
    if (hashSize == net->suffixLen) {
        if (MEMCMP((const char *) net->suffixPtr, MAINNET_GENESIS_HASH, hashSize) == 0) {
            denom = COIN_MAINNET_DENOM;
        } else if (MEMCMP((const char *) net->suffixPtr, TESTNET_GENESIS_HASH, hashSize) == 0) {
            denom = COIN_TESTNET_DENOM;
        }
    }

    if(is_negative){
        snprintf(outVal, outValLen, "%s -", denom);
    } else {
        snprintf(outVal, outValLen, "%s +", denom);
    }
    outVal += strlen(denom) + 2;
    outValLen -= strlen(denom) + 2;

    char bignum[160] = {0};
    union {
        // overlapping arrays to avoid excessive stack usage. Do not use at the same time
        uint8_t bcd[80];
        char output[160];
    } overlapped;

    MEMZERO(&overlapped, sizeof(overlapped));

    if (!format_quantity(q, overlapped.bcd, sizeof(overlapped.bcd), bignum, sizeof(bignum))) {
        return parser_unexpected_value;
    }

    fpstr_to_str(overlapped.output, sizeof(overlapped.output), bignum, COIN_AMOUNT_DECIMAL_PLACES);
    number_inplace_trimming(overlapped.output, 1);
    pageString(outVal, outValLen, overlapped.output, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printShares(const quantity_t *q,
                                             char *outVal, uint16_t outValLen,
                                             uint8_t pageIdx, uint8_t *pageCount) {
    // upperbound 2**(64*8)
    // results in 155 decimal digits => max 78 bcd bytes

    // Too many digits, we cannot format this
    LESS_THAN_64_DIGIT(q->len)

    char bignum[160] = {0};
    union {
        // overlapping arrays to avoid excessive stack usage. Do not use at the same time
        uint8_t bcd[80];
        char output[160];
    } overlapped;

    MEMZERO(&overlapped, sizeof(overlapped));

    if (!format_quantity(q, overlapped.bcd, sizeof(overlapped.bcd), bignum, sizeof(bignum))) {
        return parser_unexpected_value;
    }

    fpstr_to_str(overlapped.output, sizeof(overlapped.output), bignum, 0);
    number_inplace_trimming(overlapped.output, 1);
    pageString(outVal, outValLen, overlapped.output, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printRate(const quantity_t *q,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {

    // Too many digits, we cannot format this
    LESS_THAN_64_DIGIT(q->len)

    char bignum[160] = {0};
    union {
        // overlapping arrays to avoid excessive stack usage. Do not use at the same time
        uint8_t bcd[80];
        char output[160];
    } overlapped;

    MEMZERO(&overlapped, sizeof(overlapped));

    if (!format_quantity(q, overlapped.bcd, sizeof(overlapped.bcd), bignum, sizeof(bignum))) {
        return parser_unexpected_value;
    }

    fpstr_to_str(overlapped.output, sizeof(overlapped.output), bignum, COIN_RATE_DECIMAL_PLACES - 2);
    overlapped.output[strlen(overlapped.output)] = '%';
    pageString(outVal, outValLen, overlapped.output, pageIdx, pageCount);

    return parser_ok;
}

__Z_INLINE parser_error_t parser_printAddress(const address_raw_t *addressRaw,
                                              char *outVal, uint16_t outValLen,
                                              uint8_t pageIdx, uint8_t *pageCount, bool pt_render) {
    char outBuffer[128] = {0};

    //  and encode as bech32
    const zxerr_t err = bech32EncodeFromBytes(
            outBuffer, sizeof(outBuffer),
            COIN_HRP,
            (uint8_t *) addressRaw, sizeof(address_raw_t), 1, BECH32_ENCODING_BECH32);
    if (err != zxerr_ok) {
        return parser_invalid_address;
    }

    // Render specific addresses
    if (pt_render) {
        for (size_t i = 0; i < array_length(paraTime_lookup_helper); i++) {
            if (!MEMCMP(outBuffer, PIC(paraTime_lookup_helper[i].address), sizeof(outBuffer))) {
                *pageCount = 1;
                pageString(outVal, outValLen, (char *)PIC(paraTime_lookup_helper[i].name), pageIdx, pageCount);
                return parser_ok;
            }
        }
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printPublicKey(const publickey_t *pk,
                                                char *outVal, uint16_t outValLen,
                                                uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[128] = {0};

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), (uint8_t *) pk, 32) != 64) {
        return parser_unexpected_value;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printPublicKey_b64(const publickey_t *pk,
                                                char *outVal, uint16_t outValLen,
                                                uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[128] = {0};

    if (base64_encode(outBuffer, sizeof(outBuffer), (uint8_t *) pk, 32) != 44) {
        return parser_unexpected_value;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printVote(const uint8_t vote, char *outVal, uint16_t outValLen) {
    switch (vote) {
        case 1:
            snprintf(outVal, outValLen, "yes");
            return parser_ok;
        case 2:
            snprintf(outVal, outValLen, "no");
            return parser_ok;
        case 3:
            snprintf(outVal, outValLen, "abstain");
            return parser_ok;
        default:
            break;
    }
    return parser_unexpected_value;
}

__Z_INLINE parser_error_t parser_printVersion(const version_t ver, char *outVal, uint16_t outValLen) {
    char majorStr[5] = {0};
    char minorStr[5] = {0};
    char patchStr[5] = {0};

    uint64_to_str(majorStr, 5, ver.major);
    uint64_to_str(minorStr, 5, ver.minor);
    uint64_to_str(patchStr, 5, ver.patch);

    snprintf(outVal, outValLen, "%s.%s.%s", majorStr, minorStr, patchStr);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_ethMapNative(const uint8_t *ethstr, const uint16_t ethstrLen, uint8_t *native, const uint16_t nativeLen) {
    
    if (nativeLen < ADDR_RAW || native == NULL) {
        return parser_unexpected_value;
    }
    
    uint8_t ethaddr[ETH_ADDR_LEN] = {0};
    uint8_t total[60] = {0};
    uint8_t messageDigest[KECCAK256_HASH_LEN] = {0};

    //Take Eth addr without 0x
    hexstr_to_array(ethaddr, ETH_ADDR_LEN, (char *)&ethstr[2], ethstrLen - 2);
    // Concatenate Eth Context + version (= 0) + ethaddr (more info on oasis-SDK)
    MEMCPY(total, addressV0_Secp256k1_ethContext, sizeof(addressV0_Secp256k1_ethContext));
    MEMCPY(total+sizeof(addressV0_Secp256k1_ethContext), ethaddr, ETH_ADDR_LEN);
    SHA512_256(total, sizeof(total), messageDigest);
    
    uint8_t addressRaw[ADDR_RAW] = {0};
    MEMCPY(addressRaw + 1, messageDigest, ETH_ADDR_LEN);

    if (MEMCMP(addressRaw, native, ADDR_RAW) == 0) {
        return parser_ok;
    }

    return parser_invalid_eth_mapping;
}

__Z_INLINE parser_error_t parser_getItemRuntime(const parser_context_t *ctx,
                                               const int8_t displayIdx,
                                               char *outKey, uint16_t outKeyLen,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "To");
            snprintf(outVal, outValLen, "Self");
            if (parser_tx_obj.oasis.runtime.meta.has_orig_to) {
                if (parser_tx_obj.oasis.runtime.call.body.unencrypted.has_to) {
                    CHECK_PARSER_ERR(parser_ethMapNative((uint8_t *)parser_tx_obj.oasis.runtime.meta.orig_to, 
                    sizeof(parser_tx_obj.oasis.runtime.meta.orig_to), (uint8_t *)parser_tx_obj.oasis.runtime.call.body.unencrypted.to, sizeof(parser_tx_obj.oasis.runtime.call.body.unencrypted.to)));
                    pageStringExt(outVal, outValLen, (char *)parser_tx_obj.oasis.runtime.meta.orig_to, 42, pageIdx, pageCount);
                    return parser_ok;
                }
            } else if (parser_tx_obj.oasis.runtime.call.body.unencrypted.has_to) {
                        return parser_printAddress(&parser_tx_obj.oasis.runtime.call.body.unencrypted.to,
                            outVal, outValLen, pageIdx, pageCount, false);
            } 
            return parser_ok;
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printRuntimeQuantity(&parser_tx_obj.oasis.runtime.meta,
                                        &parser_tx_obj.oasis.runtime.call.body.unencrypted.amount,
                                        &parser_tx_obj.oasis.runtime.call.body.unencrypted.denom,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 3: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printRuntimeQuantity(&parser_tx_obj.oasis.runtime.meta,
                                        &parser_tx_obj.oasis.runtime.ai.fee.amount,
                                         &parser_tx_obj.oasis.runtime.ai.fee.denom,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 4: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.runtime.ai.fee.gas);
            *pageCount = 1;
            return parser_ok;
        }
        case 5: {
            snprintf(outKey, outKeyLen, "Runtime");
            for (size_t i = 0; i < array_length(paraTime_lookup_helper); i++) {
                if (MEMCMP(&parser_tx_obj.oasis.runtime.meta.runtime_id, PIC(paraTime_lookup_helper[i].runid),
                    sizeof(parser_tx_obj.oasis.runtime.meta.runtime_id)) == 0) {
                    pageString(outVal, outValLen, (char *)PIC(paraTime_lookup_helper[i].name), pageIdx, pageCount);
                   return parser_ok;
                }
            }
            pageString(outVal, outValLen, (char *)parser_tx_obj.oasis.runtime.meta.runtime_id, pageIdx, pageCount);
            return parser_ok;
        }
        default:
            break;
    }

    *pageCount = 0;
    return parser_no_data;
}

__Z_INLINE parser_error_t parser_getItemEntity(const oasis_entity_t *entity,
                                               int8_t displayIdx,
                                               char *outKey, uint16_t outKeyLen,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
#define ENTITY_DYNAMIC_OFFSET 1

    if (displayIdx == 0) {
        snprintf(outKey, outKeyLen, "ID");
        return parser_printPublicKey_b64(&entity->obj.id,
                                     outVal, outValLen, pageIdx, pageCount);
    }

    if (displayIdx - ENTITY_DYNAMIC_OFFSET < (int) entity->obj.nodes_length) {
        const int8_t index = displayIdx - ENTITY_DYNAMIC_OFFSET;

        snprintf(outKey, outKeyLen, "Node [%d]", index + 1);

        publickey_t node;
        CHECK_PARSER_ERR(_getEntityNodesIdAtIndex(entity, &node, index))
        return parser_printPublicKey_b64(&node, outVal, outValLen, pageIdx, pageCount);
    }

    return parser_no_data;
}

__Z_INLINE parser_error_t parser_getItemEntityMetadata(const oasis_entity_metadata_t *entity_metadata,
                                               int8_t displayIdx,
                                               char *outKey, uint16_t outKeyLen,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {

    uint8_t skipped = 0;

    if (displayIdx == 0) {
        snprintf(outKey, outKeyLen, "Version");
        uint64_to_str(outVal, outValLen, entity_metadata->v);
        *pageCount = 1;
        return parser_ok;
    }

    if (displayIdx == 1) {
        snprintf(outKey, outKeyLen, "Serial");
        uint64_to_str(outVal, outValLen, entity_metadata->serial);
        *pageCount = 1;
        return parser_ok;
    }

    if (entity_metadata->name.len > 0 && displayIdx < 3) {
        zemu_log_stack((char *) entity_metadata->name.buffer);
        size_t len = asciify((char *) entity_metadata->name.buffer);

        snprintf(outKey, outKeyLen, "Name");
        pageStringExt(outVal, outValLen, (char *) entity_metadata->name.buffer, len, pageIdx, pageCount);
        return parser_ok;
    }

    if (entity_metadata->name.len == 0)
      skipped++;

    if (entity_metadata->url.len > 0 && (displayIdx+skipped) < 4) {
      snprintf(outKey, outKeyLen, "URL");
      pageStringExt(outVal, outValLen, (char *) entity_metadata->url.buffer, entity_metadata->url.len, pageIdx, pageCount);
      return parser_ok;
    }

    if (entity_metadata->url.len == 0)
      skipped++;

    if (entity_metadata->email.len > 0 && (displayIdx+skipped) < 5) {
      snprintf(outKey, outKeyLen, "Email");
      snprintf(outVal, outValLen, "%s", entity_metadata->email.buffer);
      *pageCount = 1;
      return parser_ok;
    }

    if (entity_metadata->email.len == 0)
      skipped++;

    if (entity_metadata->keybase.len > 0 && (displayIdx+skipped) < 6) {
      snprintf(outKey, outKeyLen, "Keybase");
      snprintf(outVal, outValLen, "%s", entity_metadata->keybase.buffer);
      *pageCount = 1;
      return parser_ok;
    }

    if (entity_metadata->keybase.len == 0)
      skipped++;

    if (entity_metadata->twitter.len > 0) {
      snprintf(outKey, outKeyLen, "Twitter");
      snprintf(outVal, outValLen, "%s", entity_metadata->twitter.buffer);
      *pageCount = 1;
      return parser_ok;
    }

    return parser_no_data;
}

__Z_INLINE parser_error_t parser_printStakingTransfer(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "To");
            return parser_printAddress(&parser_tx_obj.oasis.tx.body.stakingTransfer.to,
                                        outVal, outValLen, pageIdx, pageCount, false);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.body.stakingTransfer.amount,
                                        &parser_tx_obj.context, outVal, outValLen, pageIdx, pageCount);
        }
        case 3: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx,pageCount);
        }
        case 4: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printStakingBurn(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.body.stakingBurn.amount,
                                        &parser_tx_obj.context, outVal, outValLen, pageIdx, pageCount);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 3: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printStakingWithdraw(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "From");
            return parser_printAddress(&parser_tx_obj.oasis.tx.body.stakingWithdraw.from,
                                        outVal, outValLen, pageIdx, pageCount, false);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.body.stakingWithdraw.amount,
                                        &parser_tx_obj.context, outVal, outValLen, pageIdx, pageCount);
        }
        case 3: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 4: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printStakingAllow(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Beneficiary");
            return parser_printAddress(&parser_tx_obj.oasis.tx.body.stakingAllow.beneficiary,
                                        outVal, outValLen, pageIdx, pageCount, true);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Amount change");
            return parser_printQuantityWithSign(&parser_tx_obj.oasis.tx.body.stakingAllow.amount_change,
                                                &parser_tx_obj.context, parser_tx_obj.oasis.tx.body.stakingAllow.negative,
                                                outVal, outValLen, pageIdx, pageCount);
        }
        case 3: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 4: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printStakingEscrow(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "To");
            return parser_printAddress(&parser_tx_obj.oasis.tx.body.stakingEscrow.account,
                                        outVal, outValLen, pageIdx, pageCount, false);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.body.stakingEscrow.amount,
                                        &parser_tx_obj.context, outVal, outValLen, pageIdx, pageCount);
        }
        case 3: {
            // ??? displayIdx == 1 && parser_tx_obj.oasis.tx.has_fee
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 4: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printStakingReclaimEscrow(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "From");
            return parser_printAddress(&parser_tx_obj.oasis.tx.body.stakingReclaimEscrow.account,
                                        outVal, outValLen, pageIdx, pageCount, false);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Shares");
            return parser_printShares(&parser_tx_obj.oasis.tx.body.stakingReclaimEscrow.shares,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 3: {
            // ??? displayIdx == 1 && parser_tx_obj.oasis.tx.has_fee
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 4: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printStakingAmendCommissionSchedule(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    if(displayIdx == 0){
        snprintf(outKey, outKeyLen, "Type");
        *pageCount = 1;
        return parser_getType(ctx, outVal, outValLen);
    }

    uint8_t dynDisplayIdx = displayIdx - 1;
    if( dynDisplayIdx < (int) ( parser_tx_obj.oasis.tx.body.stakingAmendCommissionSchedule.rates_length * 2 + parser_tx_obj.oasis.tx.body.stakingAmendCommissionSchedule.bounds_length * 3 ) ){
        if (dynDisplayIdx / 2 < (int) parser_tx_obj.oasis.tx.body.stakingAmendCommissionSchedule.rates_length) {
            const int8_t index = dynDisplayIdx / 2;
            commissionRateStep_t rate;

            CHECK_PARSER_ERR(_getCommissionRateStepAtIndex(ctx, &rate, index))

            switch (dynDisplayIdx % 2) {
                case 0: {
                    snprintf(outKey, outKeyLen, "Rates (%d): start", index + 1);
                    uint64_to_str(outVal, outValLen, rate.start);
                    *pageCount = 1;
                    return parser_ok;
                }
                case 1: {
                    snprintf(outKey, outKeyLen, "Rates (%d): rate", index + 1);
                    return parser_printRate(&rate.rate, outVal, outValLen, pageIdx, pageCount);
                }
            }
        } else {
            const int8_t index = (dynDisplayIdx -
                                    parser_tx_obj.oasis.tx.body.stakingAmendCommissionSchedule.rates_length * 2) / 3;

            // Only keeping one amendment in body at the time
            commissionRateBoundStep_t bound;
            CHECK_PARSER_ERR(_getCommissionBoundStepAtIndex(ctx, &bound, index))

            switch ((dynDisplayIdx -
                        parser_tx_obj.oasis.tx.body.stakingAmendCommissionSchedule.rates_length * 2) % 3) {
                case 0: {
                    snprintf(outKey, outKeyLen, "Bounds (%d): start", index + 1);
                    uint64_to_str(outVal, outValLen, bound.start);
                    *pageCount = 1;
                    return parser_ok;
                }
                case 1: {
                    snprintf(outKey, outKeyLen, "Bounds (%d): min", index + 1);
                    return parser_printRate(&bound.rate_min, outVal, outValLen, pageIdx, pageCount);
                }
                case 2: {
                    snprintf(outKey, outKeyLen, "Bounds (%d): max", index + 1);
                    return parser_printRate(&bound.rate_max, outVal, outValLen, pageIdx, pageCount);
                }
            }
        }
    }

    uint8_t lastDisplayIdx = dynDisplayIdx - parser_tx_obj.oasis.tx.body.stakingAmendCommissionSchedule.rates_length * 2 - parser_tx_obj.oasis.tx.body.stakingAmendCommissionSchedule.bounds_length * 3;
    switch (lastDisplayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printgRegistryDeregisterEntity(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
            outVal, outValLen, pageIdx, pageCount);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printRegistryUnfreezeNode(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        case 3:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printPublicKey(
                    &parser_tx_obj.oasis.tx.body.registryUnfreezeNode.node_id,
                    outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printRegisterEntity(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    if(displayIdx == 0){
        snprintf(outKey, outKeyLen, "Type");
        *pageCount = 1;
        return parser_getType(ctx, outVal, outValLen);
    }

    int8_t dynDisplayIdx = displayIdx - 1;
    if(dynDisplayIdx < ( (int) parser_tx_obj.oasis.tx.body.registryRegisterEntity.entity.obj.nodes_length + ENTITY_DYNAMIC_OFFSET ) ){
        return parser_getItemEntity(
                    &parser_tx_obj.oasis.tx.body.registryRegisterEntity.entity,
                    dynDisplayIdx,
                    outKey, outKeyLen, outVal, outValLen,
                    pageIdx, pageCount);
    }

    dynDisplayIdx = dynDisplayIdx - parser_tx_obj.oasis.tx.body.registryRegisterEntity.entity.obj.nodes_length - ENTITY_DYNAMIC_OFFSET;
    switch (dynDisplayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printGovernanceCastVote(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Type");
            *pageCount = 1;
            return parser_getType(ctx, outVal, outValLen);
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Proposal ID");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.body.governanceCastVote.id);
            *pageCount = 1;
            return parser_ok;
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Vote");
            *pageCount = 1;
            return parser_printVote(parser_tx_obj.oasis.tx.body.governanceCastVote.vote, outVal, outValLen);
        }
        case 3: {
            snprintf(outKey, outKeyLen, "Fee");
            return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                        outVal, outValLen, pageIdx, pageCount);
        }
        case 4: {
            snprintf(outKey, outKeyLen, "Gas limit");
            uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
            *pageCount = 1;
            return parser_ok;
        }
        default:
            break;
    }
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_printGovernanceSubmitProposal(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    if(displayIdx == 0) {
        snprintf(outKey, outKeyLen, "Type");
        *pageCount = 1;
        return parser_getType(ctx, outVal, outValLen);
    }

    if(parser_tx_obj.oasis.tx.body.governanceSubmitProposal.type == upgrade ){
        switch (displayIdx) {
            case 1: {
                snprintf(outKey, outKeyLen, "Kind");
                *pageCount = 1;
                snprintf(outVal, outValLen, "Upgrade");
                return parser_ok;
            }
            case 2:{
                snprintf(outKey, outKeyLen, "Handler");
                *pageCount = 1;
                snprintf(outVal, outValLen, "%s", parser_tx_obj.oasis.tx.body.governanceSubmitProposal.upgrade.handler);
                return parser_ok;
            }

            case 3:{
                snprintf(outKey, outKeyLen, "Consensus");
                *pageCount = 1;
                return parser_printVersion(parser_tx_obj.oasis.tx.body.governanceSubmitProposal.upgrade.target.consensus_protocol, outVal, outValLen);
            }
            case 4:{
                snprintf(outKey, outKeyLen, "Runtime Host");
                *pageCount = 1;
                return parser_printVersion(parser_tx_obj.oasis.tx.body.governanceSubmitProposal.upgrade.target.runtime_host_protocol, outVal, outValLen);
            }
            case 5:{
                snprintf(outKey, outKeyLen, "Runtime Committee");
                *pageCount = 1;
                return parser_printVersion(parser_tx_obj.oasis.tx.body.governanceSubmitProposal.upgrade.target.runtime_committee_protocol, outVal, outValLen);
            }
            case 6:{
                snprintf(outKey, outKeyLen, "Epoch");
                *pageCount = 1;
                uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.body.governanceSubmitProposal.upgrade.epoch);
                return parser_ok;
            }
            case 7: {
                snprintf(outKey, outKeyLen, "Fee");
                return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                            outVal, outValLen, pageIdx, pageCount);
            }
            case 8: {
                snprintf(outKey, outKeyLen, "Gas limit");
                uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
                *pageCount = 1;
                return parser_ok;
            }
        }
    } else if(parser_tx_obj.oasis.tx.body.governanceSubmitProposal.type == cancelUpgrade ){
        switch (displayIdx) {
            case 1: {
                snprintf(outKey, outKeyLen, "Kind");
                *pageCount = 1;
                snprintf(outVal, outValLen, "Cancel upgrade");
                return parser_ok;
            }
            case 2:{
                snprintf(outKey, outKeyLen, "Proposal ID");
                uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.body.governanceSubmitProposal.cancel_upgrade.proposal_id);
                *pageCount = 1;
                return parser_ok;
            }
            case 3: {
                snprintf(outKey, outKeyLen, "Fee");
                return parser_printQuantity(&parser_tx_obj.oasis.tx.fee_amount, &parser_tx_obj.context,
                                            outVal, outValLen, pageIdx, pageCount);
            }
            case 4: {
                snprintf(outKey, outKeyLen, "Gas limit");
                uint64_to_str(outVal, outValLen, parser_tx_obj.oasis.tx.fee_gas);
                *pageCount = 1;
                return parser_ok;
            }
        }
    }
            
    return parser_display_idx_out_of_range;
}

__Z_INLINE parser_error_t parser_getItemTx(const parser_context_t *ctx,
                                           int8_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
                                            
    // Variable items
    switch (parser_tx_obj.oasis.tx.method) {
        case stakingTransfer:
            return parser_printStakingTransfer(ctx, displayIdx, outKey, outKeyLen,
                                            outVal, outValLen, pageIdx, pageCount);
        case stakingBurn:
            return parser_printStakingBurn(ctx, displayIdx, outKey, outKeyLen,
                                            outVal, outValLen, pageIdx, pageCount);
        case stakingWithdraw:
            return parser_printStakingWithdraw(ctx, displayIdx, outKey, outKeyLen,
                                            outVal, outValLen, pageIdx, pageCount);
        case stakingAllow:
            return parser_printStakingAllow(ctx, displayIdx, outKey, outKeyLen,
                                            outVal, outValLen, pageIdx, pageCount);
        case stakingEscrow:
            return parser_printStakingEscrow(ctx, displayIdx, outKey, outKeyLen,
                                            outVal, outValLen, pageIdx, pageCount);
        case stakingReclaimEscrow:
            return parser_printStakingReclaimEscrow(ctx, displayIdx, outKey, outKeyLen,
                                                    outVal, outValLen, pageIdx, pageCount);
        case stakingAmendCommissionSchedule:
            return parser_printStakingAmendCommissionSchedule(ctx, displayIdx, outKey, outKeyLen,
                                                            outVal, outValLen, pageIdx, pageCount);
        case registryDeregisterEntity:
            return parser_printgRegistryDeregisterEntity(ctx, displayIdx, outKey, outKeyLen,
                                                        outVal, outValLen, pageIdx, pageCount);
        case registryUnfreezeNode:
            return parser_printRegistryUnfreezeNode(ctx, displayIdx, outKey, outKeyLen,
                                                    outVal, outValLen, pageIdx, pageCount);
        case registryRegisterEntity:
            return parser_printRegisterEntity(ctx, displayIdx, outKey, outKeyLen,
                                                outVal, outValLen, pageIdx, pageCount);
        case governanceCastVote:
            return parser_printGovernanceCastVote(ctx, displayIdx, outKey, outKeyLen,
                                                    outVal, outValLen, pageIdx, pageCount);
        case governanceSubmitProposal:
            return parser_printGovernanceSubmitProposal(ctx, displayIdx, outKey, outKeyLen,
                                                        outVal, outValLen, pageIdx, pageCount);
        case unknownMethod:
        default:
            break;
    }

    *pageCount = 0;
    return parser_no_data;
}

parser_error_t parser_getItem(const parser_context_t *ctx,
                              uint16_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);
    snprintf(outKey, outKeyLen, "?");
    snprintf(outVal, outValLen, " ");
    *pageCount = 0;

    uint8_t numItems = 0;
    CHECK_PARSER_ERR(parser_getNumItems(ctx, &numItems))
    CHECK_APP_CANARY()

    if (numItems == 0) {
        return parser_unexpected_number_items;
    }

    if (displayIdx < 0 || displayIdx >= numItems) {
        return parser_no_data;
    }

    parser_error_t err = parser_ok;

    if (parser_tx_obj.context.suffixLen > 0 && displayIdx + 1 == numItems /*last*/) {
        // Display context
        snprintf(outKey, outKeyLen, "Network");
        const uint8_t hashSize = sizeof(MAINNET_GENESIS_HASH) - 1;
        if ((hashSize == parser_tx_obj.context.suffixLen && MEMCMP((const char *) parser_tx_obj.context.suffixPtr, MAINNET_GENESIS_HASH,
                   parser_tx_obj.context.suffixLen) == 0) || (hashSize == sizeof(parser_tx_obj.oasis.runtime.meta.chain_context) &&
                   MEMCMP(parser_tx_obj.oasis.runtime.meta.chain_context, MAINNET_GENESIS_HASH,
                   sizeof(parser_tx_obj.oasis.runtime.meta.chain_context)) == 0)) {
            *pageCount = 1;
            snprintf(outVal, outValLen, "Mainnet");
        } else if ((hashSize == parser_tx_obj.context.suffixLen && MEMCMP((const char *) parser_tx_obj.context.suffixPtr, TESTNET_GENESIS_HASH,
                   parser_tx_obj.context.suffixLen) == 0) || (hashSize == sizeof(parser_tx_obj.oasis.runtime.meta.chain_context) &&
                   MEMCMP(parser_tx_obj.oasis.runtime.meta.chain_context, TESTNET_GENESIS_HASH,
                   sizeof(parser_tx_obj.oasis.runtime.meta.chain_context)) == 0)) {
            *pageCount = 1;
            snprintf(outVal, outValLen, "Testnet");
        } else {
        pageStringExt(outVal, outValLen,
                      (const char *) parser_tx_obj.context.suffixPtr, parser_tx_obj.context.suffixLen,
                      pageIdx, pageCount);
        }
    } else {
        switch (parser_tx_obj.type) {
            case txType: {
                err = parser_getItemTx(ctx,
                                       displayIdx,
                                       outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
                break;
            }
            case entityType: {
                if (displayIdx == 0) {
                    snprintf(outKey, outKeyLen, "Sign");
                    *pageCount = 1;
                    snprintf(outVal, outValLen, "Entity");
                } else {
                    err = parser_getItemEntity(&parser_tx_obj.oasis.entity,
                                               displayIdx - 1,
                                               outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
                }
                break;
            }
            case entityMetadataType: {
                if (displayIdx == 0) {
                    snprintf(outKey, outKeyLen, "Sign");
                    *pageCount = 1;
                    snprintf(outVal, outValLen, "Entity metadata");
                } else {
                    err = parser_getItemEntityMetadata(&parser_tx_obj.oasis.entity_metadata,
                                               displayIdx - 1,
                                               outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
                }
                break;
            }
            case runtimeType: {
                err = parser_getItemRuntime(ctx,
                                       displayIdx,
                                       outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
                break;
            }

            default:
                return parser_unexpected_type;
        }
    }


    #if defined(NO_DISPLAY)
        // Add paging values on cpp tests
        if (err == parser_ok && *pageCount > 1) {
            size_t keyLen = strlen(outKey);
            if (keyLen < outKeyLen) {
                snprintf(outKey + keyLen, outKeyLen - keyLen, "[%d/%d]", pageIdx + 1, *pageCount);
            }
        }
    #endif

    return err;
}

#endif // APP_CONSUMER
