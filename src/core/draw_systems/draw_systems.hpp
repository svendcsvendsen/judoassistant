#pragma once

#include "core/draw_systems/draw_system.hpp"
#include "core/draw_systems/knockout_draw_system.hpp"
#include "core/draw_systems/pool_draw_system.hpp"

class DrawSystems {
public:
    static const std::vector<std::unique_ptr<DrawSystem>> & getDrawSystems();
private:
};