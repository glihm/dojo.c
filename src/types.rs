use std::sync::Arc;

use starknet::{
    accounts::SingleOwnerAccount,
    providers::{jsonrpc::HttpTransport, JsonRpcClient},
    signers::LocalWallet,
};
use stream_cancel::Trigger;
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct Provider(pub(crate) Arc<JsonRpcClient<HttpTransport>>);
#[wasm_bindgen]
pub struct Account(pub(crate) SingleOwnerAccount<Arc<JsonRpcClient<HttpTransport>>, LocalWallet>);
#[wasm_bindgen]
pub struct Subscription(pub(crate) Trigger);
