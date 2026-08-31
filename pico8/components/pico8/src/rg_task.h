// rg_task.h
// ============================================================================
// Thin compatibility shim.
//
// `rg_task.h` is missing from the user's `components/retro-go/` clone, but
// the entire rg_task API is already declared inside `rg_system.h`. We provide
// this shim so that consumers (notably retro_go_backend.c in our component) can
// `#include <rg_task.h>` and pick up the rg_task_* symbols / types they need:
//
//   - struct rg_task_s / typedef rg_task_t
//   - typedef enum rg_task_priority_t (RG_TASK_PRIORITY_*, RG_TASK_MSG_STOP, etc.)
//   - rg_task_create / rg_task_find / rg_task_current
//   - rg_task_delay / rg_task_yield
//   - rg_task_send / rg_task_peek / rg_task_receive
//   - rg_task_is_blocked / rg_task_messages_waiting
//
// If/when an upstream retro-go drops in a real rg_task.h, this shim should be
// deleted; it intentionally does not re-declare any symbol to avoid clashes.
// ============================================================================

#pragma once

#include "rg_system.h"
