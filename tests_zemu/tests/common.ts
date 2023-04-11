import { DEFAULT_START_OPTIONS, IDeviceModel } from '@zondax/zemu'

const APP_SEED = "equip will roof matter pink blind book anxiety banner elbow sun young"

const Resolve = require('path').resolve

const APP_PATH_S = Resolve('../app/output/app_s.elf')
const APP_PATH_X = Resolve('../app/output/app_x.elf')
const APP_PATH_SP = Resolve('../app/output/app_s2.elf')
const APP_PATH_ST = Resolve('../app/output/app_stax.elf')

export const models: IDeviceModel[] = [
  { name: 'nanos', prefix: 'S', path: APP_PATH_S },
  { name: 'nanox', prefix: 'X', path: APP_PATH_X },
  { name: 'nanosp', prefix: 'SP', path: APP_PATH_SP },
  { name: 'stax', prefix: 'ST', path: APP_PATH_ST },
]

export const defaultOptions = {
  ...DEFAULT_START_OPTIONS,
  logging: true,
  custom: `-s "${APP_SEED}"`
};

export const STANDARD_BLOBS = [
  {
    name: "sign_basic_withdraw",
    context: "oasis-core/consensus: tx for chain bc1c715319132305795fa86bd32e93291aaacbfb5b5955f3ba78bdba413af9e1",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omRmcm9tVQAGaeylE0pICHuqRvArp3IYjeXN22ZhbW91bnRAZW5vbmNlAGZtZXRob2Rwc3Rha2luZy5XaXRoZHJhdw==",
      "base64"),
  },
  {
    name: "sign_basic_allow",
    context: "oasis-core/consensus: tx for chain bc1c715319132305795fa86bd32e93291aaacbfb5b5955f3ba78bdba413af9e1",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omtiZW5lZmljaWFyeVUA8PesI5mFWUkMVHwStQ6Fieb4bsFtYW1vdW50X2NoYW5nZUBlbm9uY2UBZm1ldGhvZG1zdGFraW5nLkFsbG93",
      "base64"),
  },
  {
    name: "sign_basic",
    context: "oasis-core/consensus: tx for chain testing",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omJ0b1UAxzzAAUY0NJFbo/OXUb63wJBbRetmYW1vdW50QGVub25jZQBmbWV0aG9kcHN0YWtpbmcuVHJhbnNmZXI=",
      "base64"),
  },
  {
    name: "submit_proposal_upgrade",
    context: "oasis-core/consensus: tx for chain 31baebfc917e608ab5d26d8e072d70627cdef4df342b98bb61fe3683e4e4b2ac",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5oWd1cGdyYWRlpGF2AWVlcG9jaBv//////////mZ0YXJnZXSjcmNvbnNlbnN1c19wcm90b2NvbKJlbWlub3IMZXBhdGNoAXVydW50aW1lX2hvc3RfcHJvdG9jb2yjZW1ham9yAWVtaW5vcgJlcGF0Y2gDeBpydW50aW1lX2NvbW1pdHRlZV9wcm90b2NvbKJlbWFqb3IYKmVwYXRjaAFnaGFuZGxlcnJkZXNjcmlwdG9yLWhhbmRsZXJlbm9uY2UAZm1ldGhvZHgZZ292ZXJuYW5jZS5TdWJtaXRQcm9wb3NhbA==",
      "base64"),
  },
  {
    name: "submit_proposal_cancel_upgrade",
    context: "oasis-core/consensus: tx for chain 31baebfc917e608ab5d26d8e072d70627cdef4df342b98bb61fe3683e4e4b2ac",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5oW5jYW5jZWxfdXBncmFkZaFrcHJvcG9zYWxfaWQb//////////9lbm9uY2UBZm1ldGhvZHgZZ292ZXJuYW5jZS5TdWJtaXRQcm9wb3NhbA==",
      "base64"),
  },
  {
    name: "add_escrow",
    context: "oasis-core/consensus: tx for chain bc1c715319132305795fa86bd32e93291aaacbfb5b5955f3ba78bdba413af9e1",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omZhbW91bnRCA+hnYWNjb3VudFUAWjuJqnaIaHy9a5gyaOTU0hR4ladlbm9uY2UAZm1ldGhvZHFzdGFraW5nLkFkZEVzY3Jvdw==",
      "base64"),
  },
  {
    name: "reclaim_escrow",
    context: "oasis-core/consensus: tx for chain bc1c715319132305795fa86bd32e93291aaacbfb5b5955f3ba78bdba413af9e1",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcxkD6GZhbW91bnRAZGJvZHmiZnNoYXJlc0BnYWNjb3VudFUAcFT2Pq0X0FM7dpOkrzN8jWLiu4Blbm9uY2UBZm1ldGhvZHVzdGFraW5nLlJlY2xhaW1Fc2Nyb3c=",
      "base64"),
  },
  {
    name: "transfer",
    context: "oasis-core/consensus: tx for chain bc1c715319132305795fa86bd32e93291aaacbfb5b5955f3ba78bdba413af9e1",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcxkD6GZhbW91bnRAZGJvZHmiYnRvVQDHPMABRjQ0kVuj85dRvrfAkFtF62ZhbW91bnRAZW5vbmNlCmZtZXRob2Rwc3Rha2luZy5UcmFuc2Zlcg==",
      "base64"),
  },
  {
    name: "cast_vote_abstain",
    context: "oasis-core/consensus: tx for chain 31baebfc917e608ab5d26d8e072d70627cdef4df342b98bb61fe3683e4e4b2ac",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omJpZABkdm90ZQNlbm9uY2UBZm1ldGhvZHNnb3Zlcm5hbmNlLkNhc3RWb3Rl",
      "base64"),
  },
  {
    name: "cast_vote_yes",
    context: "oasis-core/consensus: tx for chain 31baebfc917e608ab5d26d8e072d70627cdef4df342b98bb61fe3683e4e4b2ac",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omJpZBoAmJaAZHZvdGUBZW5vbmNlAWZtZXRob2RzZ292ZXJuYW5jZS5DYXN0Vm90ZQ==",
      "base64"),
  },
  {
    name: "cast_vote_no",
    context: "oasis-core/consensus: tx for chain 31baebfc917e608ab5d26d8e072d70627cdef4df342b98bb61fe3683e4e4b2ac",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omJpZBv//////////2R2b3RlAmVub25jZQFmbWV0aG9kc2dvdmVybmFuY2UuQ2FzdFZvdGU=",
      "base64"),
  },
  {
    name: "sign_amend",
    context: "oasis-core/consensus: tx for chain testing amend",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcxkD6GZhbW91bnRAZGJvZHmhaWFtZW5kbWVudKJlcmF0ZXOFomRyYXRlQicQZXN0YXJ0GQPoomRyYXRlQicQZXN0YXJ0GQPoomRyYXRlQicQZXN0YXJ0GQPoomRyYXRlQicQZXN0YXJ0GQPoomRyYXRlQicQZXN0YXJ0GQPoZmJvdW5kc4WjZXN0YXJ0GQPoaHJhdGVfbWF4QicQaHJhdGVfbWluQicQo2VzdGFydBkD6GhyYXRlX21heEInEGhyYXRlX21pbkInEKNlc3RhcnQZA+hocmF0ZV9tYXhCJxBocmF0ZV9taW5CJxCjZXN0YXJ0GQPoaHJhdGVfbWF4QicQaHJhdGVfbWluQicQo2VzdGFydBkD6GhyYXRlX21heEInEGhyYXRlX21pbkInEGVub25jZRkD6GZtZXRob2R4H3N0YWtpbmcuQW1lbmRDb21taXNzaW9uU2NoZWR1bGU=",
      "base64"),
  },
  {
    name: "sign_amend_issue_130",
    context: "oasis-core/consensus: tx for chain testing amend",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcxkD6GZhbW91bnRCB9BkYm9keaFpYW1lbmRtZW50omVyYXRlc4WiZHJhdGVCw1Blc3RhcnQYIKJkcmF0ZUKcQGVzdGFydBgoomRyYXRlQnUwZXN0YXJ0GDCiZHJhdGVCYahlc3RhcnQYOKJkcmF0ZUJOIGVzdGFydBhAZmJvdW5kc4KjZXN0YXJ0GCBocmF0ZV9tYXhCw1BocmF0ZV9taW5CJxCjZXN0YXJ0GEBocmF0ZV9tYXhCdTBocmF0ZV9taW5CJxBlbm9uY2UYKmZtZXRob2R4H3N0YWtpbmcuQW1lbmRDb21taXNzaW9uU2NoZWR1bGU=",
      "base64"),
  },
  {
    name: "sign_entity_metadata",
    context: "oasis-metadata-registry: entity",
    txBlob: Buffer.from(
      "a76176016375726c7568747470733a2f2f6d792e656e746974792f75726c646e616d656e4d7920656e74697479206e616d6565656d61696c6d6d7940656e746974792e6f72676673657269616c01676b657962617365716d795f6b6579626173655f68616e646c656774776974746572716d795f747769747465725f68616e646c65==",
      "hex"),
  },
  {
    name: "sign_entity_descriptor",
    context: "oasis-core/registry: register entity",
    txBlob: Buffer.from(
      "a2617602626964582097e72e6e83ec39eb98d7e9189513aba662a08a210b9974b0f7197458483c7161",
      "hex"),
  },
  {
    name: "sign_register_entity",
    context: "oasis-core/consensus: tx for chain 265bbfc4e631486af2d846e8dfb3aa67ab379e18eb911a056e7ab38e3934a9a5",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcxkD6GZhbW91bnRCEJJkYm9keaJpc2lnbmF0dXJlomlzaWduYXR1cmVYQOMUbqcZ88wjPfkp9/poVJJKrxD/UN7qvEQQi5O5D7IFD7ujuBsfaCkbuXtFUMIrlAbsC9RuvLPcsPQqm5jIWApqcHVibGljX2tleVgg3wNS/vFr/qqy6oAqgBzesWUMhZB7C8DnCED4T/NKy6NzdW50cnVzdGVkX3Jhd192YWx1ZVjao2F2AmJpZFgg3wNS/vFr/qqy6oAqgBzesWUMhZB7C8DnCED4T/NKy6Nlbm9kZXOFWCB45nhPM7JsZvqGq/ODtQGz93xlsv51iRbAQP+5dCy/ZVggnc1QiTMP4HFSTpiEiEtnFDRtYxKRc85PTTR8SlH1g8JYIKx4PngeJJhJb+tTC7IR/BWh5mTB+uJOLY04HFA3DwGcWCCZkWVMaW1btF+krrmppVsALHVwxbj5pu0nj6HgADbN91ggobWw+PHovNp7UxCZ0yw8lbbs5jTvV5vY3zu7yrVxVD9lbm9uY2Ub//////////9mbWV0aG9kd3JlZ2lzdHJ5LlJlZ2lzdGVyRW50aXR5",
      "base64"),
  },
  {
    name: "sign_register_entity_no_nodes",
    context: "oasis-core/consensus: tx for chain 265bbfc4e631486af2d846e8dfb3aa67ab379e18eb911a056e7ab38e3934a9a5",
    txBlob: Buffer.from(
      "pGNmZWWiY2dhcwBmYW1vdW50QGRib2R5omlzaWduYXR1cmWiaXNpZ25hdHVyZVhACXa0AUpgRcGA0PHYSaX7p+tMiVqo6NkQH5TY9WuGz84SuZCYpzoNsijTTzCQYp5Xc6T9b2Ew9TIoz9E7JvvSBmpwdWJsaWNfa2V5WCDfA1L+8Wv+qrLqgCqAHN6xZQyFkHsLwOcIQPhP80rLo3N1bnRydXN0ZWRfcmF3X3ZhbHVlWCmiYXYCYmlkWCDfA1L+8Wv+qrLqgCqAHN6xZQyFkHsLwOcIQPhP80rLo2Vub25jZQBmbWV0aG9kd3JlZ2lzdHJ5LlJlZ2lzdGVyRW50aXR5",
      "base64"),
  },
  {
    name: "sign_deregister_entity",
    context: "oasis-core/consensus: tx for chain 265bbfc4e631486af2d846e8dfb3aa67ab379e18eb911a056e7ab38e3934a9a5",
    txBlob: Buffer.from(
      "a363666565a2636761731903e866616d6f756e744207d0656e6f6e6365182a666d6574686f64781972656769737472792e44657265676973746572456e74697479",
      "hex"),
  },
  {
    name: "sign_entity_metadata_utf8",
    context: "oasis-metadata-registry: entity",
    txBlob: Buffer.from(
      "A76176016375726C7568747470733A2F2F6D792E656E746974792F75726C646E616D656868C3A96CC3A86E6565656D61696C6D6D7940656E746974792E6F72676673657269616C01676B657962617365716D795F6B6579626173655F68616E646C656774776974746572716D795F747769747465725F68616E646C65",
      "hex"),
  },
  {
    name: "sign_entity_metadata_long",
    context: "oasis-metadata-registry: entity",
    txBlob: Buffer.from(
      "a76176016375726c783f68747470733a2f2f6d792e656e746974792f75726c2f746869732f69732f736f6d652f766572792f6c6f6e672f76616c69642f75726c2f75702f746f2f3634646e616d6578315468697320697320736f6d652076657279206c6f6e6720656e74697479206e616d65206275742076616c6964202835302965656d61696c6d6d7940656e746974792e6f72676673657269616c01676b657962617365716d795f6b6579626173655f68616e646c656774776974746572716d795f747769747465725f68616e646c65",
      "hex"),
  },
]
