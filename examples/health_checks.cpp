#include <service_app/service_app.hpp>

#include <iostream>

using namespace vix::service_app;

int main()
{
  ServiceApp app;

  ServiceInfo info;
  info.name = "health-example";
  info.version = "0.1.0";
  info.environment = "dev";

  app.set_info(info);

  // Database check example
  app.add_health_check("database", []()
                       {
        bool db_ok = true;

        if (!db_ok)
        {
            return HealthCheckResult{
                "database",
                HealthStatus::Unhealthy,
                "db connection failed"};
        }

        return HealthCheckResult{
            "database",
            HealthStatus::Healthy,
            "ok"}; });

  // Cache check example
  app.add_health_check("cache", []()
                       { return HealthCheckResult{
                             "cache",
                             HealthStatus::Healthy,
                             "ok"}; });

  std::cout << "Health checks registered\n";

  return 0;
}
