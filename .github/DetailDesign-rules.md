# Detailed Design Rules

1. **Required content:** The design MUST document architecture, types, classes, functions, data flow, external interactions, and requirement traceability relevant to the task.
2. **Diagrams:** All diagrams MUST use Mermaid syntax and be embedded in Markdown code blocks.
3. **Class diagrams:** MUST describe each designed class's members, methods, and relationships with other classes.
4. **Flowcharts:** MUST describe the internal control flow of each designed function with nontrivial logic.
5. **Type tables:** MUST define each type required by the implementation, including its name, purpose, members where applicable, and usage.
6. **Sequence diagrams:** MUST describe external call order, participating modules, and the functions invoked across module boundaries.
7. **Single responsibility:** Each designed class, function, and member variable MUST have a distinct, requirement-related responsibility. Avoid redundant or unnecessary elements.
8. **Traceability:** Every design element implementing an upstream requirement MUST identify its source requirement ID. Use a traceability table mapping requirement IDs to design elements. Preserve IDs exactly.
9. **Implementation readiness:** The design MUST provide sufficient detail to implement the solution without inventing unspecified behavior. Explicitly identify unresolved requirements and assumptions.

Include only applicable design elements, but do not omit a required diagram or table when the corresponding functionality exists.