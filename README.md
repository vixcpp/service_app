# service_app

Production service application kit for modern C++.

`service_app` provides a minimal, deterministic service layer built on top of `vix/api_app`.

It enables:

- Service metadata
- Health endpoints
- Readiness checks
- Service lifecycle hooks
- Metrics snapshot
- Graceful shutdown signal

Header-only. Layered. Explicit.

## Download

https://vixcpp.com/registry/pkg/vix/service_app

## Why service_app?

Most production services need infrastructure beyond basic HTTP routing:

- Health endpoints for orchestration
- Readiness checks for dependencies
- Service metadata
- Graceful shutdown
- Operational metrics
- Consistent lifecycle management

In many C++ backends these concerns are:

- Mixed with business logic
- Implemented differently per service
- Hard to standardize
- Difficult to test

`service_app` provides:

- Standard service endpoints
- Deterministic health checks
- Simple lifecycle hooks
- Minimal metrics snapshot
- Explicit service metadata

No hidden threads.
No monitoring stack required.
No infrastructure lock-in.

Just a clean production service foundation.

## Dependency

`service_app` depends on:

- `vix/api_app`
- (transitively) `vix/web_app`
- (transitively) `vix/app`

Architecture layering:

```
vix/app
  ↑
vix/web_app
  ↑
vix/api_app
  ↑
vix/service_app
```

This ensures:

- Clear service architecture
- Stable infrastructure layers
- Minimal dependencies
- Deterministic behavior

Dependencies are installed automatically via Vix Registry.

## Installation

### Using Vix Registry

```bash
vix add vix/service_app
vix deps
```

### Manual

```bash
git clone https://github.com/vixcpp/service_app.git
```

Add the `include/` directory and ensure dependencies are available.

## Core concepts

### Service metadata

Every service exposes basic metadata:

```cpp
ServiceInfo info;
info.name = "orders-service";
info.version = "1.0.0";
info.environment = "production";
info.instance_id = "orders-1";

app.set_info(info);
```

The `/info` endpoint returns this information.

### Health checks

Health checks verify the internal state of the service:

```cpp
app.add_health_check("database", []() {
    return HealthCheckResult{
        "database",
        HealthStatus::Healthy,
        "ok"
    };
});
```

Endpoint:

- `GET /health`

Example response:

```json
{
  "status": "ok",
  "checks": [
    { "name": "database", "status": "ok" }
  ]
}
```

### Readiness checks

Readiness checks determine if the service can accept traffic:

```cpp
app.add_readiness_check("cache", []() {
    return HealthCheckResult{
        "cache",
        HealthStatus::Healthy,
        "ok"
    };
});
```

Endpoint:

- `GET /ready`

### Lifecycle hooks

Hooks allow executing logic during startup or shutdown.

Startup example:

```cpp
app.on_startup([]() {
    // initialize resources
});
```

Shutdown example:

```cpp
app.on_shutdown([]() {
    // release resources
});
```

Typical flow:

```cpp
app.register_service_routes();
app.run_startup();

/* start HTTP server */

app.run_shutdown();
```

### Graceful shutdown

A stop request can be issued safely:

```cpp
app.request_stop();

if (app.stop_requested())
{
    // stop server loop
}
```

This allows controlled shutdown of the service.

### Metrics snapshot

`service_app` tracks basic service metrics:

- total requests
- total errors
- health checks
- readiness checks
- uptime

Example metrics export hook:

```cpp
app.on_metrics([](const ServiceMetrics& m) {
    std::cout << "Requests: " << m.requests_total << "\n";
});
```

Metrics can be exported by calling:

```cpp
app.export_metrics();
```

The kit does not enforce a metrics format.

## Standard endpoints

`service_app` registers these endpoints:

| Endpoint   | Purpose |
|-----------|---------|
| `/health` | Liveness check |
| `/ready`  | Readiness check |
| `/info`   | Service metadata |
| `/metrics`| Metrics snapshot |

These endpoints are deterministic and synchronous.

## Main service flow

Typical service execution:

1. Configure service metadata
2. Register health checks
3. Register API routes
4. Register service routes
5. Run startup hook
6. Start HTTP runtime
7. Handle requests
8. Shutdown gracefully

## Complexity

| Operation                  | Complexity |
|---------------------------|------------|
| Health check evaluation    | O(n) |
| Readiness check evaluation | O(n) |
| Metrics snapshot           | O(1) |
| Request dispatch           | depends on router |

Health checks should remain lightweight and deterministic.

## Design philosophy

`service_app` focuses on:

- Deterministic service infrastructure
- Explicit lifecycle management
- Minimal runtime overhead
- Clear separation of service concerns
- Lightweight operational tooling

It does not attempt to replace:

- Full monitoring stacks
- Distributed tracing systems
- Service meshes
- External health orchestration

Those belong to infrastructure layers outside the application.

## Tests

Run:

```bash
vix build
vix test
```

Tests verify:

- service metadata
- lifecycle hooks
- health checks
- graceful shutdown
- metrics snapshot

## License

MIT License\
Copyright (c) Gaspard Kirira

