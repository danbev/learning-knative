#[macro_use]
extern crate serde_derive;

use kube::{
    api::{Object, RawApi, Informer, WatchEvent, Void},
    client::APIClient,
    config,
};

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct Something {
    pub title: String,
    pub description: String,
}

type KubeSomething = Object<Something, Void>;

fn main() {
    let _config_options = config::ConfigOptions{
        cluster: Some("api.crc.testing:6443".to_string()),
        context: Some("developer".to_string()),
        user: Some("kube:admin/api-crc-testing:6443".to_string()),
    };
    let kubeconfig = config::load_kube_config()
        .expect("kubeconfig failed to load");
    println!("base_path: {:?}", kubeconfig.base_path);
    println!("Client: {:?}", kubeconfig.client);
    let client = APIClient::new(kubeconfig);
    let namespace = "default";
    let resource = RawApi::customResource("somethings")
        .group("example.nodeshift")
        .within(&namespace);

    let informer = Informer::raw(client, resource).init().expect("informer init failed");

    loop {
        informer.poll().expect("informer poll failed");
        while let Some(event) = informer.pop() {
            handle(event);
        }
    }
}

fn handle(event: WatchEvent<KubeSomething>) {
    println!("Something happened to a something resource instance: {:?}", event)
}
