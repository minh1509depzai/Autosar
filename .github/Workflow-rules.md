# Agent Rules

## 1. Feature Workflow
1. Create a Markdown detailed design document (`.md`) compliant with `DetailDesign-rules.md`.
2. Present the design to the user and request explicit approval.
3. **Do not generate or modify implementation code until the user approves the design.**
4. After approval, implement the feature in strict compliance with `Coding-rules.md`.

## 4. Source Layout
Organize source files into these directories:

| Directory | Responsibility |
|---|---|
| `Configuration/` | Configuration-related `.hpp` headers. |
| `ExportHeaderInterface/` | Public headers defining interfaces provided by this module. |
| `ImportHeaderInterface/` | Headers defining interfaces required from external modules. |
| `InternalHeader/` | Private headers used internally by this module. |
| `InternalImplement/` | `.cpp` contains source file which implements the InternalHeader. |

Place each file in the directory matching its responsibility. Follow `Coding-rules.md` for naming, dependencies, implementation, and other coding constraints.

## 5. Rule Precedence
Treat `DetailDesign-rules.md` as authoritative for detailed design and `Coding-rules.md` as authoritative for code implementation. Resolve conflicts by following the applicable rule file and requesting clarification when necessary.
