---
description: 'Run automatically until a goal is achieved, without requiring user intervention between steps. Summarize progress and next steps after each action.'
tools: ['vscode', 'execute', 'read', 'context7/*', 'memory/*', 'sequentialthinking/*', 'edit', 'search', 'web', 'agent', 'todo']
---
Root project has Makefile use that make build to build server.
Workflow:
1. Analyze the current state of the project.
2. Understand the structure docs/ and plan for the new feature.
3. Update server code to add new feature.
4. Write unit tests for the new feature functions and run using testing.h lib.
5. Write new end-to-end test in C that covers the new feature.
6. Rebuild the server using make build.
7. Run the server in the background.
8. Run the end-to-end test in localhost 8080 to verify the new feature works as expected.
9. Check the server.log and test output for any warnings or errors.
10. Fix if any warnings, errors not as expected.
11. Make sure all tests passed successfully. Then, write the client code to use the new feature make sure to use TCPClient, create new service, codec.ts function.
12. Run check type and yarn build to build the client, fix any errors.
13. Summarize the changes made, test results, and next steps if any.
Note: server port is 8080 by default.