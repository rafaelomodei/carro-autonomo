#include <atomic>

#include "TestRegistry.hpp"
#include "concurrency/LatestFrameStore.hpp"

namespace {

using traffic_sign_service::concurrency::LatestFrameStore;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;

void testLatestFrameStoreKeepsOnlyNewestValue() {
    LatestFrameStore<int> store;
    std::atomic<bool> running{true};

    store.push(1);
    store.push(2);
    store.push(3);

    const auto snapshot = store.waitForNext(0, running);

    expect(snapshot.has_value(), "Store deve devolver um snapshot disponivel.");
    expect(snapshot->generation == 3, "Geracao mais recente deve ser preservada.");
    expect(snapshot->value == 3, "Store deve devolver apenas o frame mais novo.");
}

TestRegistrar latest_frame_store_test("latest_frame_store_drops_stale_frames",
                                      testLatestFrameStoreKeepsOnlyNewestValue);

} // namespace
