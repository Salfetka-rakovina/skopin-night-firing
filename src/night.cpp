#include "skopin/night.hpp"
#include "skopin/kiln.hpp"

#include <unordered_set>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::vector<Ware> remaining;
    std::unordered_set<std::string> seen_marks;

    for (const auto& ware : queue) {
        if (seen_marks.count(ware.mark) > 0) {
            report.refused.push_back(Refusal{ware.mark, "duplicate"});
            continue;
        }
        seen_marks.insert(ware.mark);

        if (validateWare(ware).has_value()) {
            report.refused.push_back(Refusal{ware.mark, "invalid"});
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back(Refusal{ware.mark, "thick"});
            continue;
        }
        remaining.push_back(ware);
    }

    Kiln kiln;

    while (!remaining.empty()) {
        std::vector<Ware> batch;
        batch.push_back(remaining.front());
        remaining.erase(remaining.begin());

        if (batch.front().kind != WareKind::Sagger) {
            bool skipped_by_height = false;
            std::size_t i = 0;
            while (i < remaining.size()) {
                const Ware& candidate = remaining[i];
                if (candidate.program != batch.front().program ||
                    candidate.kind == WareKind::Sagger) {
                    break;
                }
                if (skipped_by_height && candidate.kind != WareKind::Rush) {
                    ++i;
                    continue;
                }
                if (fitsInKiln(batch, candidate)) {
                    batch.push_back(candidate);
                    remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                } else {
                    skipped_by_height = true;
                    ++i;
                }
            }
        }

        if (kiln.door() == Door::Closed) {
            kiln.open();
        }
        for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
            kiln.load(*it);
        }
        kiln.close();

        if (batch.front().program == Program::Bisque) {
            kiln.fire();
            const bool next_is_glaze =
                !remaining.empty() && remaining.front().program == Program::Glaze;
            if (!next_is_glaze) {
                kiln.cool();
            }
        } else {
            while (kiln.cone() != Cone::Glaze) {
                kiln.fire();
            }
            kiln.cool();
            kiln.cool();
        }

        kiln.open();
        for (const auto& ware : batch) {
            kiln.unload(ware.mark);
            report.passed.push_back(ware.mark);
        }
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin