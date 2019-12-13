#[macro_use]
extern crate serde_derive;

use kube::{
    api::{Informer, Object, RawApi, Void, WatchEvent},
    client::APIClient,
    config,
};

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct Member {
    pub title: String,
    pub description: String,
}

type KubeMember = Object<Member, Void>;

fn main() {
    let kubeconfig = config::load_kube_config().expect("kubeconfig failed to load");

    let client = APIClient::new(kubeconfig);

    let namespace = "default";
    let resource = RawApi::customResource("members")
        .group("example.nodeshift.com")
        .within(&namespace);

    let informer = Informer::raw(client, resource)
        .init()
        .expect("informer init failed");
    loop {
        informer.poll().expect("informer poll failed");

        while let Some(event) = informer.pop() {
            handle(event);
        }
    }
}

fn handle(event: WatchEvent<KubeMember>) {
    match event {
        WatchEvent::Added(m) => println!(
            "Added member {} with title '{}' ({})",
            m.metadata.name, m.spec.title, m.spec.description
        ),
        WatchEvent::Deleted(m) => println!("Deleted {} {}",
                                           m.spec.title,
                                           m.metadata.name),
        _ => println!("another event"),
    }
}
