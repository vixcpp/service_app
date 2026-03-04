#include <service_app/service_app.hpp>

#include <cassert>
#include <iostream>

using namespace vix::service_app;

int main()
{
  ServiceApp app;

  // Configure service metadata
  ServiceInfo info;
  info.name = "test-service";
  info.version = "1.0.0";
  info.environment = "test";
  info.instance_id = "instance-1";

  app.set_info(info);

  const auto &cfg = app.info();

  assert(cfg.name == "test-service");
  assert(cfg.version == "1.0.0");
  assert(cfg.environment == "test");
  assert(cfg.instance_id == "instance-1");

  // Add health check
  app.add_health_check("basic", []()
                       { return HealthCheckResult{
                             "basic",
                             HealthStatus::Healthy,
                             "ok"}; });

  // Startup / shutdown hooks
  bool started = false;
  bool stopped = false;

  app.on_startup([&]()
                 { started = true; });

  app.on_shutdown([&]()
                  { stopped = true; });

  app.run_startup();
  assert(started);

  app.request_stop();
  assert(app.stop_requested());

  app.run_shutdown();
  assert(stopped);

  std::cout << "service_app basic test passed\n";
}
