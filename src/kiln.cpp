#include "skopin/kiln.hpp"

namespace skopin {

Kiln::Kiln() = default;

Cone Kiln::cone() const {
    return cone_;
}

Door Kiln::door() const {
    return door_;
}

int Kiln::stokes() const {
    return stokes_;
}

std::vector<Ware> Kiln::stack() const {
    std::vector<Ware> result;
    result.reserve(items_.size());
    for (const auto& slot : items_) {
        result.push_back(slot.ware);
    }
    return result;
}

void Kiln::open() {
    if (door_ == Door::Open) {
        throw KilnError("door is already open");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("cannot open door at glaze cone");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("door is already closed");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ != Door::Closed) {
        throw KilnError("door must be closed to fire");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("already at glaze cone");
    }
    for (const auto& slot : items_) {
        if (slot.ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("ware is too thick to fire");
        }
    }
    if (cone_ == Cone::Bisque) {
        for (const auto& slot : items_) {
            if (slot.ware.program == Program::Bisque) {
                throw KilnError("cannot fire bisque ware to glaze cone");
            }
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else if (cone_ == Cone::Bisque) {
        cone_ = Cone::Glaze;
        for (auto& slot : items_) {
            slot.saw_glaze_cone = true;
        }
    }
    ++stokes_;
}

void Kiln::cool() {
    if (door_ != Door::Closed) {
        throw KilnError("door must be closed to cool");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("already at cold cone");
    }
    for (const auto& slot : items_) {
        if (slot.ware.program == Program::Glaze && !slot.saw_glaze_cone) {
            throw KilnError("cannot cool unseen glaze ware");
        }
    }

    if (cone_ == Cone::Glaze) {
        cone_ = Cone::Bisque;
    } else if (cone_ == Cone::Bisque) {
        cone_ = Cone::Cold;
    }
    ++stokes_;
}

void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open) {
        throw KilnError("door must be open to load");
    }
    if (validateWare(ware).has_value()) {
        throw KilnError("invalid ware");
    }
    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("only glaze ware can be loaded at bisque cone");
    }
    for (const auto& slot : items_) {
        if (slot.ware.mark == ware.mark) {
            throw KilnError("duplicate mark in kiln");
        }
    }
    if (!items_.empty()) {
        if (ware.kind == WareKind::Sagger || items_.front().ware.kind == WareKind::Sagger) {
            throw KilnError("sagger occupies the kiln alone");
        }
    }
    if (!fitsInKiln(stack(), ware)) {
        throw KilnError("ware does not fit in kiln");
    }

    items_.push_back(Slot{ware, false});
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("door must be open to unload");
    }
    if (items_.empty()) {
        throw KilnError("kiln is empty");
    }
    const Ware& top = items_.back().ware;
    if (top.mark != mark) {
        throw KilnError("mark does not match top of stack");
    }
    if (top.program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("glaze ware can only be unloaded at cold cone");
    }

    items_.pop_back();
}

}  // namespace skopin