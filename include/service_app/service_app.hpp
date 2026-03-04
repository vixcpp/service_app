/**
 * @file service_app.hpp
 * @brief Production service application kit built on top of vix/api_app.
 *
 * `service_app` provides a minimal production-oriented service layer for JSON-first backends:
 *
 * - Service metadata (name, version, instance id, environment)
 * - Health and readiness endpoints (/health, /ready)
 * - Startup and shutdown hooks
 * - Graceful stop helper (request stop from outside)
 * - Metrics hook (push-style callback, no built-in exporter)
 * - Deterministic, explicit behavior
 *
 * This kit is intentionally lightweight:
 * - No mandatory JSON library dependency
 * - No metrics format enforced (Prometheus/OpenMetrics left to users)
 * - No background threads created automatically
 * - No networking runtime included (handled by web_app transport)
 *
 * Requirements: C++17+
 * Header-only. Depends on `vix/api_app`.
 */

#ifndef VIX_SERVICE_APP_SERVICE_APP_HPP
#define VIX_SERVICE_APP_SERVICE_APP_HPP

#include <api_app/api_app.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::service_app
{

  /**
   * @brief Minimal service metadata exposed by info endpoints.
   */
  struct ServiceInfo
  {
    std::string name = "service";
    std::string version = "0.1.0";
    std::string environment = "dev"; // dev, staging, prod
    std::string instance_id = "local";
    std::uint64_t started_at_ms = 0; // epoch ms (optional)
  };

  /**
   * @brief Health status for health/readiness.
   */
  enum class HealthStatus
  {
    Healthy,
    Unhealthy
  };

  /**
   * @brief Health check result.
   */
  struct HealthCheckResult
  {
    std::string name;
    HealthStatus status = HealthStatus::Healthy;
    std::string message; // optional human readable info
  };

  /**
   * @brief Health check function.
   *
   * Must be deterministic and fast.
   * No IO is enforced by this kit.
   */
  using HealthCheck = std::function<HealthCheckResult()>;

  /**
   * @brief Minimal service metrics snapshot.
   *
   * Export format is user-defined (JSON, text, Prometheus).
   */
  struct ServiceMetrics
  {
    std::uint64_t requests_total = 0;
    std::uint64_t errors_total = 0;
    std::uint64_t health_checks_total = 0;
    std::uint64_t readiness_checks_total = 0;

    std::uint64_t uptime_ms = 0;
  };

  /**
   * @brief Service application built on top of vix/api_app.
   *
   * This is a production service kit:
   * it provides standard endpoints and lifecycle helpers.
   *
   * Notes:
   * - It does not start a server by itself (web_app transport does that).
   * - It can be used as a base for your HTTP runtime binding.
   */
  class ServiceApp : public vix::api_app::ApiApplication
  {
  public:
    using Hook = std::function<void()>;
    using MetricsHook = std::function<void(const ServiceMetrics &)>;

    ServiceApp() = default;
    ~ServiceApp() override = default;

    ServiceApp(const ServiceApp &) = delete;
    ServiceApp &operator=(const ServiceApp &) = delete;

    ServiceApp(ServiceApp &&) = delete;
    ServiceApp &operator=(ServiceApp &&) = delete;

    /**
     * @brief Set service metadata.
     */
    void set_info(ServiceInfo info)
    {
      info_ = std::move(info);
    }

    /**
     * @brief Get service metadata.
     */
    const ServiceInfo &info() const noexcept
    {
      return info_;
    }

    /**
     * @brief Add a health check.
     *
     * Health checks are used for /health and /ready.
     * You can also register separate checks for readiness only.
     */
    void add_health_check(std::string name, HealthCheck fn)
    {
      if (name.empty())
        throw std::runtime_error("ServiceApp::add_health_check: name must not be empty");
      if (!fn)
        throw std::runtime_error("ServiceApp::add_health_check: fn must be set");

      health_checks_.push_back(CheckEntry{std::move(name), std::move(fn)});
    }

    /**
     * @brief Add a readiness-only check.
     *
     * These checks run only for /ready.
     */
    void add_readiness_check(std::string name, HealthCheck fn)
    {
      if (name.empty())
        throw std::runtime_error("ServiceApp::add_readiness_check: name must not be empty");
      if (!fn)
        throw std::runtime_error("ServiceApp::add_readiness_check: fn must be set");

      readiness_checks_.push_back(CheckEntry{std::move(name), std::move(fn)});
    }

    /**
     * @brief Set startup hook executed once before serving requests.
     *
     * Call run_startup() from your runtime after binding routes and before serving.
     */
    void on_startup(Hook h)
    {
      startup_hook_ = std::move(h);
    }

    /**
     * @brief Set shutdown hook executed once during stop.
     */
    void on_shutdown(Hook h)
    {
      shutdown_hook_ = std::move(h);
    }

    /**
     * @brief Set metrics export hook (push callback).
     *
     * The kit does not schedule exports. Your runtime decides when to call export_metrics().
     */
    void on_metrics(MetricsHook h)
    {
      metrics_hook_ = std::move(h);
    }

    /**
     * @brief Request the service to stop (graceful).
     *
     * This sets an internal flag you can poll from your server loop.
     * It does not force-stop threads.
     */
    void request_stop() noexcept
    {
      stop_requested_ = true;
    }

    /**
     * @brief Check if stop was requested.
     */
    bool stop_requested() const noexcept
    {
      return stop_requested_;
    }

    /**
     * @brief Register standard production endpoints.
     *
     * Endpoints:
     * - GET /health
     * - GET /ready
     * - GET /info
     * - GET /metrics (optional, JSON snapshot)
     *
     * This assumes your web_app router supports GET registration.
     * If your WebApplication uses a different API, adapt this function accordingly.
     */
    void register_service_routes()
    {
      auto &r = this->router();

      r.get("/health", [&](const vix::web_app::Request &)
            {
      metrics_.health_checks_total++;
      auto res = eval_checks(health_checks_, false);
      return health_response(res); });

      r.get("/ready", [&](const vix::web_app::Request &)
            {
      metrics_.readiness_checks_total++;
      auto a = eval_checks(health_checks_, true);
      auto b = eval_checks(readiness_checks_, true);
      a.details.insert(a.details.end(), b.details.begin(), b.details.end());
      a.ok = a.ok && b.ok;
      return readiness_response(a); });

      r.get("/info", [&](const vix::web_app::Request &)
            { return info_response(); });

      r.get("/metrics", [&](const vix::web_app::Request &)
            { return metrics_response_json(); });
    }

    /**
     * @brief Call startup hook once.
     *
     * Typical usage:
     * - configure routes
     * - register_service_routes()
     * - run_startup()
     * - start server loop
     */
    void run_startup()
    {
      started_at_ = std::chrono::steady_clock::now();

      if (info_.started_at_ms == 0)
        info_.started_at_ms = now_epoch_ms();

      if (startup_hook_)
        startup_hook_();
    }

    /**
     * @brief Call shutdown hook once.
     */
    void run_shutdown()
    {
      if (shutdown_hook_)
        shutdown_hook_();
    }

    /**
     * @brief Export metrics via hook (push).
     */
    void export_metrics()
    {
      if (!metrics_hook_)
        return;

      ServiceMetrics snap = metrics_;
      snap.uptime_ms = uptime_ms();
      metrics_hook_(snap);
    }

    /**
     * @brief Dispatch request with service counters and api error mapping.
     */
    vix::web_app::Response dispatch_service(const vix::web_app::Request &req) const
    {
      metrics_.requests_total++;

      auto r = dispatch_api(req);
      if (r.status >= 400)
        metrics_.errors_total++;

      return r;
    }

  private:
    struct CheckEntry
    {
      std::string name;
      HealthCheck fn;
    };

    struct CheckEval
    {
      bool ok = true;
      std::vector<HealthCheckResult> details;
    };

    static std::uint64_t now_epoch_ms()
    {
      using namespace std::chrono;
      return static_cast<std::uint64_t>(
          duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    }

    std::uint64_t uptime_ms() const
    {
      if (!started_at_.has_value())
        return 0;

      const auto d = std::chrono::steady_clock::now() - *started_at_;
      return static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::milliseconds>(d).count());
    }

    static CheckEval eval_checks(const std::vector<CheckEntry> &checks, bool)
    {
      CheckEval out;

      for (const auto &c : checks)
      {
        HealthCheckResult r;
        try
        {
          r = c.fn ? c.fn() : HealthCheckResult{c.name, HealthStatus::Unhealthy, "missing check"};
          if (r.name.empty())
            r.name = c.name;
        }
        catch (const std::exception &e)
        {
          r = HealthCheckResult{c.name, HealthStatus::Unhealthy, e.what()};
        }
        catch (...)
        {
          r = HealthCheckResult{c.name, HealthStatus::Unhealthy, "unknown error"};
        }

        out.details.push_back(r);
        if (r.status != HealthStatus::Healthy)
          out.ok = false;
      }

      return out;
    }

    vix::web_app::Response info_response() const
    {
      std::string body;
      body.reserve(info_.name.size() + info_.version.size() + info_.environment.size() + info_.instance_id.size() + 128);

      body += "{\"service\":{";
      body += "\"name\":\"";
      body += vix::api_app::json_escape(info_.name);
      body += "\",\"version\":\"";
      body += vix::api_app::json_escape(info_.version);
      body += "\",\"environment\":\"";
      body += vix::api_app::json_escape(info_.environment);
      body += "\",\"instance_id\":\"";
      body += vix::api_app::json_escape(info_.instance_id);
      body += "\",\"started_at_ms\":";
      body += std::to_string(info_.started_at_ms);
      body += "}}";

      return vix::api_app::ApiApplication::ok_json(std::move(body));
    }

    static vix::web_app::Response health_response(const CheckEval &e)
    {
      // 200 if ok, else 503
      const int status = e.ok ? 200 : 503;

      std::string body;
      body.reserve(128 + e.details.size() * 96);

      body += "{\"status\":\"";
      body += (e.ok ? "ok" : "fail");
      body += "\",\"checks\":[";

      for (std::size_t i = 0; i < e.details.size(); ++i)
      {
        const auto &c = e.details[i];
        if (i)
          body += ",";

        body += "{\"name\":\"";
        body += vix::api_app::json_escape(c.name);
        body += "\",\"status\":\"";
        body += (c.status == HealthStatus::Healthy ? "ok" : "fail");
        body += "\"";

        if (!c.message.empty())
        {
          body += ",\"message\":\"";
          body += vix::api_app::json_escape(c.message);
          body += "\"";
        }

        body += "}";
      }

      body += "]}";

      return vix::api_app::ApiApplication::json_response(status, std::move(body));
    }

    static vix::web_app::Response readiness_response(const CheckEval &e)
    {
      // same shape as health
      return health_response(e);
    }

    vix::web_app::Response metrics_response_json() const
    {
      ServiceMetrics snap = metrics_;
      snap.uptime_ms = uptime_ms();

      std::string body;
      body.reserve(256);

      body += "{\"metrics\":{";
      body += "\"requests_total\":";
      body += std::to_string(snap.requests_total);
      body += ",\"errors_total\":";
      body += std::to_string(snap.errors_total);
      body += ",\"health_checks_total\":";
      body += std::to_string(snap.health_checks_total);
      body += ",\"readiness_checks_total\":";
      body += std::to_string(snap.readiness_checks_total);
      body += ",\"uptime_ms\":";
      body += std::to_string(snap.uptime_ms);
      body += "}}";

      return vix::api_app::ApiApplication::ok_json(std::move(body));
    }

  private:
    ServiceInfo info_{};

    std::vector<CheckEntry> health_checks_{};
    std::vector<CheckEntry> readiness_checks_{};

    Hook startup_hook_{};
    Hook shutdown_hook_{};
    MetricsHook metrics_hook_{};

    bool stop_requested_ = false;

    mutable ServiceMetrics metrics_{};
    std::optional<std::chrono::steady_clock::time_point> started_at_{};

  protected:
    void serve_once() override {}
  };

} // namespace vix::service_app

#endif // VIX_SERVICE_APP_SERVICE_APP_HPP
