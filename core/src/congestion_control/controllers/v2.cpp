#include <userver/congestion_control/controllers/v2.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

namespace {
constexpr std::chrono::seconds kStepPeriod{1};
}  // namespace

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats) {
  writer["is-enabled"] = stats.is_enabled ? 1 : 0;
  if (stats.current_limit) writer["current-limit"] = stats.current_limit;
  writer["enabled-seconds"] = stats.enabled_epochs;
}

Controller::Controller(const std::string& name, Sensor& sensor,
                       Limiter& limiter, Stats& stats)
    : name_(name), sensor_(sensor), limiter_(limiter), stats_(stats) {}

void Controller::Start() {
  LOG_INFO() << fmt::format("Congestion controller {} has started", name_);
  periodic_.Start("congestion_control", {kStepPeriod}, [this] { Step(); });
}

void Controller::Step() {
  auto current = sensor_.GetCurrent();
  auto limit = Update(current);
  limiter_.SetLimit(limit);

  if (limit.load_limit) {
    LOG_ERROR() << fmt::format(
        "Congestion Control {} is active, sensor ({}), limiter ({})", name_,
        current.ToLogString(), limit.ToLogString());
  }

  if (limit.load_limit.has_value()) stats_.enabled_epochs++;
  stats_.current_limit = limit.load_limit.value_or(0);
  stats_.is_enabled = limit.load_limit.has_value();
}

const std::string& Controller::GetName() const { return name_; }

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END