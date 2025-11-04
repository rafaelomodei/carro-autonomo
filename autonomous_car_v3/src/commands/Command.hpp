#pragma once

namespace autonomous_car::commands {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(double value = 0.0) = 0;
};

} // namespace autonomous_car::commands
