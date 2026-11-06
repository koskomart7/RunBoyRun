\# RunBoyRun — Claude Project Context



\## 1. Project Overview

\- Project: RunBoyRun

\- Type: Personal Unreal Engine game project

\- Original Engine: UE 5.6

\- Current Target: UE 5.8

\- Development Stage: Mid-development

\- Architecture: C++ + Blueprint

\- Version Control: GitHub Desktop



\## 2. Development Background

\- Blueprint is used for most gameplay logic.

\- C++ is used for selected components and major

&#x20; character functionality.

\- Parkour gameplay is an important development area.

\- Existing AI workflows relied on manually applying

&#x20; proposed code and Blueprint logic.

\- Incorrect assumptions sometimes caused unwanted

&#x20; changes to existing systems.



\## 3. MCP Environment

\- MCP: Monolith v0.24.0

\- Target Engine: Unreal Engine 5.8

\- AI Client: Claude Code

\- Plugin Directory: Plugins/Monolith

\- Connection: .mcp.json



Installation and connection status must be verified.



\## 4. Mandatory Development Rules

1\. Inspect existing implementation before editing.

2\. Do not assume assets, classes, or functions exist.

3\. Preserve all working gameplay logic.

4\. Explain proposed modifications before execution.

5\. Request approval for destructive changes.

6\. Edit one subsystem at a time.

7\. Compile modified Blueprints and C++ code.

8\. Validate results using PIE and editor inspection.

9\. Use screenshots when visual evidence is useful.

10\. Review Git changes before committing.



\## 5. Parkour Development

\- Inspect character movement components.

\- Inspect collision and trace systems.

\- Inspect Blueprint and C++ responsibilities.

\- Inspect animation state transitions.

\- Identify existing problems before rewriting logic.

\- Prefer minimal, testable changes.



\## 6. Known Migration History

\- Original project developed with UE 5.6.

\- Project migration to UE 5.8 initiated.

\- BuildSettingsVersion.V7 was required.



\## 7. Important Restrictions

\- Never rewrite large Blueprint graphs blindly.

\- Never modify unrelated systems.

\- Never delete project assets without approval.

\- Never claim a feature works without testing.

\- Keep Git changes small and reviewable.

\- Do not commit without explicit authorization.



\## 8. Long-term Goals

\- Improve AI-assisted Unreal development workflows.

\- Develop and refine parkour gameplay systems.

\- Use MCP for inspection, editing, and testing.

\- Monitor future Unreal Engine 6 compatibility.



\## 9. Information Still Required

\- Actual C++ class hierarchy

\- Blueprint asset paths and relationships

\- Existing parkour mechanics

\- Animation Blueprint architecture

\- Input Mapping Contexts

\- Current bugs and incomplete features

\- Verified UE 5.8 migration status



\## MCP Security Rules



\- Inspect existing assets before modification.

\- Never delete or overwrite assets without approval.

\- Do not execute destructive operations automatically.

\- Request approval before modifying critical gameplay code.

\- Never read or disclose credentials or secret files.

\- Do not make external network requests unless authorized.

\- Keep all modifications within the intended project scope.

\- Review Git changes after each operation.

\- Never commit or push without explicit permission.

\- Report all modified assets and files.

