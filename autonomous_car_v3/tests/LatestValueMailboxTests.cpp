#include <chrono>
#include <memory>

#include "TestRegistry.hpp"
#include "services/async/LatestValueMailbox.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
namespace async = autonomous_car::services::async;

void testLatestValueMailboxKeepsOnlyNewestItem() {
    async::LatestValueMailbox<int> mailbox;

    const bool replaced_first = mailbox.offer(1);
    const bool replaced_second = mailbox.offer(2);
    const auto latest_value = mailbox.tryPop();

    expect(!replaced_first, "Primeiro publish nao deve contar como descarte.");
    expect(replaced_second, "Segundo publish deve sobrescrever o item pendente.");
    expect(latest_value.has_value() && *latest_value == 2,
           "Mailbox deve entregar apenas o valor mais recente.");
    expect(!mailbox.tryPop().has_value(), "Nao deve restar backlog depois do pop.");
    expect(mailbox.droppedCount() == 1,
           "Contador de descartes deve refletir o overwrite do item pendente.");
}

void testLatestValueMailboxWaitsForNewestSharedPayload() {
    async::LatestValueMailbox<std::shared_ptr<int>> mailbox;
    mailbox.offer(std::make_shared<int>(7));

    const auto value = mailbox.waitPopFor(std::chrono::milliseconds(10), [] { return false; });

    expect(value.has_value() && *value.value() && **value == 7,
           "waitPopFor deve retornar o payload mais recente quando houver item disponivel.");
}

TestRegistrar latest_mailbox_overwrite_test("latest_value_mailbox_keeps_only_the_latest_item",
                                            testLatestValueMailboxKeepsOnlyNewestItem);
TestRegistrar latest_mailbox_wait_test("latest_value_mailbox_waits_for_latest_payload",
                                       testLatestValueMailboxWaitsForNewestSharedPayload);

} // namespace
