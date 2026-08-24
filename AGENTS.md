# AGENTS.md

Write C++ as a human-readable object model.

Rules:
- Use class inheritance for real "is-a" relationships only.
- Prefer UML-readable designs: inheritance, composition, aggregation, association.
- Every class name must state its purpose clearly.
- Use purpose-driven names like `SemanticAnalyzer`, `RuntimeEnvironment`, `ExpressionNode`, `InheritanceRelationship`.
- Avoid vague names like `Helper`, `Manager`, `Util`, `Processor`, `Thing`, `Base`, `Common`.
- Separate model classes, service classes, context classes, and relationship classes.
- Make the code easy to translate into a UML class diagram.
- Prefer explicit abstractions over clever implementation tricks.
- Avoid procedural blobs, hidden responsibilities, and cryptic generic code.
- Prefer readability over brevity.
- Refactor toward clearer names and clearer class responsibilities.
- If a design cannot be explained simply in object-model terms, rewrite it.

Naming:
- Classes: PascalCase, noun-based, purpose-revealing.
- Methods: verb-based, explicit.
- Variables: role-based, readable, not abbreviated unless trivial.
- Abstract bases should represent real conceptual categories.
- Concrete subclasses should narrow meaning clearly.

Completion standard:
- The code must read like a purpose-driven class model.
- The object relationships must be understandable to a human reviewer.
- The design must be explainable in UML terms without guessing.
