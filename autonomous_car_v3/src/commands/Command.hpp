#pragma once

namespace autonomous_car::commands {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
};

} // namespace autonomous_car::commands
