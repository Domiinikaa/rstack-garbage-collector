# Recursive Stack (`rstack`) with Mark & Sweep Garbage Collection

An advanced data structure implementation in C (C23 standard) representing a **Recursive Stack** (`rstack`). Each stack node can contain either a 64-bit unsigned integer or a pointer to another `rstack`. 

Because stacks can refer to other stacks—potentially forming directed graphs with shared nodes and cyclic references—memory management is handled via a **Mark & Sweep Garbage Collector**.

## Key Architecture & Features

### 1. Structure & Representation
* **Doubly-Linked Stack Elements:** Allows traversal from both the top (standard LIFO operations) and bottom (essential for chronological order serialization).
* **Global Node Tracking:** All allocated `rstack` instances are registered in a global doubly-linked list anchored by a sentinel node.

### 2. Mark & Sweep Garbage Collection
Instead of naive recursive freeing (which fails on shared nodes and cycles), `rstack_delete` unmarks the stack as a "root" and triggers garbage collection:
* **Mark Phase:** Traverses all active root stacks recursively and marks all reachable stacks.
* **Sweep Phase:** Iterates through the global stack registry and unlinks/frees any unreferenced (unreachable) stack instances and their memory.

### 3. Cycle Detection & Traversal Safety
Operations traversing stack graphs (such as checking if empty, reading the front value, or writing to disk) can encounter cyclic references (`A -> B -> A`):
* **Visit ID Pattern:** `rstack_empty` and `rstack_front` use a global monotonic counter (`visit_id`) to prevent infinite recursion during depth-first traversals.
* **Path Tracking:** `rstack_write` maintains an execution path stack (`path_node_t`) to detect cycles along the current branch and terminate recursion gracefully.

## API Reference

| Function | Description |
| :--- | :--- |
| `rstack_new()` | Allocates a new root `rstack` instance and registers it globally. |
| `rstack_delete(rs)` | Marks stack as non-root and triggers Mark & Sweep garbage collection. |
| `rstack_push_value(rs, val)` | Pushes a 64-bit unsigned integer onto the top of the stack. |
| `rstack_push_rstack(rs1, rs2)` | Pushes a reference to `rs2` onto `rs1`. |
| `rstack_pop(rs)` | Pops the top element from the stack. |
| `rstack_empty(rs)` | Returns `true` if the stack contains no primitive values (recursively). |
| `rstack_front(rs)` | Evaluates and returns the first primitive integer value accessible in the stack structure. |
| `rstack_read(path)` | Parses primitive integer values from a text file into a new `rstack`. |
| `rstack_write(path, rs)` | Writes the contents of the stack (bottom-to-top) to a file. |
