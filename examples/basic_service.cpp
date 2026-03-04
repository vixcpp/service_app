#include <service_app/service_app.hpp>

#include <iostream>

using namespace vix::service_app;
using namespace vix::web_app;

int main()
{
  ServiceApp app;

  ServiceInfo info;
  info.name = "example-service";
  info.version = "1.0.0";
  info.environment = "dev";
  info.instance_id = "local";

  app.set_info(info);

  // Health check
  app.add_health_check("basic", []()
                       { return HealthCheckResult{
                             "basic",
                             HealthStatus::Healthy,
                             "ok"}; });

  // API route
  app.get("/hello", [](const Request &)
          { return vix::api_app::ApiApplication::ok_json(
                "{\"message\":\"hello from service_app\"}"); });

  // Register standard service endpoints
  app.register_service_routes();

  app.run_startup();

  std::cout << "Service started\n";

  // Fake request (example only)
  Request req;
  req.method = "GET";
  req.path = "/hello";

  auto res = app.dispatch_service(req);

  std::cout << "Response status: " << res.status << "\n";
  std::cout << "Response body: " << res.body << "\n";

  app.run_shutdown();

  return 0;
}
