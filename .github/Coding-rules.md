## Naming and Traceability Rules

1. **Types:** Use `PascalCase` for class, struct, type, and interface names.
2. **Fields:** Prefix private fields with `_`; use `camelCase` for the remaining name.
3. **Constants:** Use `UPPER_SNAKE_CASE`.
4. **Functions and methods:** Use `camelCase`.
5. **Variables:** Use `camelCase`.
6. **Enum members:** Use `PascalCase`.
7. **File names:** Match the primary class, struct, or type name. If a file contains multiple types without a clear primary type, use a descriptive name reflecting its contents.
8. **AUTOSAR traceability:** For every AUTOSAR requirement implemented by a piece of logic, add a comment immediately before that logic in the form `/* @<requirement_id> */`. Add one comment per applicable requirement, each on a separate line. Preserve requirement IDs exactly.
9. **Requirement naming:** When implementing AUTOSAR requirements, use the terminology and identifiers specified by the requirements wherever practical, unless doing so conflicts with an explicit project naming rule or language constraint.