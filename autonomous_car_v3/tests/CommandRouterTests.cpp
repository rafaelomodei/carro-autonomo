#include <optional>

#include "TestRegistry.hpp"
#include "common/DrivingMode.hpp"
#include "controllers/CommandRouter.hpp"

namespace {

using autonomous_car::CommandSource;
using autonomous_car::DrivingMode;
using autonomous_car::controllers::CommandRouteStatus;
using autonomous_car::controllers::CommandRouter;
using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;

void testCommandRouterRoutesCompatibleManualCommands() {
    CommandRouter router;
    int manual_calls = 0;
    router.registerCommand("forward", CommandSource::Manual,
                           [&manual_calls](std::optional<double>) { ++manual_calls; });

    const auto status =
        router.route(CommandSource::Manual, "forward", 0.5, DrivingMode::Manual);

    expect(status == CommandRouteStatus::Handled,
           "Comando manual compatível deve ser tratado.");
    expect(manual_calls == 1, "Callback manual deve ser executado.");
}

void testCommandRouterRoutesExplicitAutonomousStartStop() {
    CommandRouter router;
    int starts = 0;
    int stops = 0;
    router.registerCommand("start", CommandSource::Autonomous,
                           [&starts](std::optional<double>) { ++starts; });
    router.registerCommand("stop", CommandSource::Autonomous,
                           [&stops](std::optional<double>) { ++stops; });

    const auto start_status =
        router.route(CommandSource::Autonomous, "start", std::nullopt,
                     DrivingMode::Autonomous);
    const auto stop_status =
        router.route(CommandSource::Autonomous, "stop", std::nullopt,
                     DrivingMode::Autonomous);

    expect(start_status == CommandRouteStatus::Handled,
           "Start autonomo deve ser tratado em modo autonomo.");
    expect(stop_status == CommandRouteStatus::Handled,
           "Stop autonomo deve ser tratado em modo autonomo.");
    expect(starts == 1 && stops == 1,
           "Callbacks de start e stop devem ser executados.");
}

void testCommandRouterRejectsIncompatibleModeAndSource() {
    CommandRouter router;
    int calls = 0;
    router.registerCommand("forward", CommandSource::Manual,
                           [&calls](std::optional<double>) { ++calls; });
    router.registerCommand("stop", CommandSource::Autonomous,
                           [&calls](std::optional<double>) { ++calls; });

    const auto manual_in_auto =
        router.route(CommandSource::Manual, "forward", 0.8, DrivingMode::Autonomous);
    const auto autonomous_stop_in_manual =
        router.route(CommandSource::Autonomous, "stop", std::nullopt, DrivingMode::Manual);
    const auto wrong_source_same_name =
        router.route(CommandSource::Manual, "stop", std::nullopt, DrivingMode::Manual);

    expect(manual_in_auto == CommandRouteStatus::RejectedByMode,
           "Comando manual deve ser rejeitado no modo autonomo.");
    expect(autonomous_stop_in_manual == CommandRouteStatus::RejectedByMode,
           "Origem autonoma deve ser rejeitada no modo manual.");
    expect(wrong_source_same_name == CommandRouteStatus::RejectedBySource,
           "Mesmo nome de comando deve respeitar a origem registrada.");
    expect(calls == 0, "Nenhum callback rejeitado deve produzir efeito colateral.");
}

TestRegistrar router_manual_test("command_router_routes_compatible_manual_commands",
                                 testCommandRouterRoutesCompatibleManualCommands);
TestRegistrar router_autonomous_test("command_router_routes_explicit_autonomous_start_stop",
                                     testCommandRouterRoutesExplicitAutonomousStartStop);
TestRegistrar router_reject_test("command_router_rejects_incompatible_mode_and_source",
                                 testCommandRouterRejectsIncompatibleModeAndSource);

} // namespace
