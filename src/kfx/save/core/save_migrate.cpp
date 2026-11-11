/******************************************************************************/
/** @file save_migrate.cpp
 *     Migrations for changes of meaning that name matching can't express.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "game_legacy.h"
#include "post_inc.h"

#include "save_migrate.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <vector>

/** Format 1.10 (the team's #5581) moved the hand from the keeper (Dungeon) to each player (UserState), and
 *  Dungeon's list became a queue of things to drop at the heart, one per turn. In an older save that list is
 *  still the hand: give it to the keeper's primary user (the lowest user index playing that keeper, the rule
 *  of get_player_primary_user), so a load keeps what was held instead of dropping it at the heart. A keeper
 *  no user plays (a computer) keeps its list. Edits this edition's own tree: the team ships no migration. */
static int migrate_hand_to_user(struct SaveLoadCtx *context)
{
    struct Game *state = static_cast<struct Game *>(context->state);
    static_assert(sizeof(state->dungeon[0].things_in_hand) == sizeof(state->user_states[0].things_in_hand),
        "the keeper's list and the user's hand must hold the same number of things");
    for (int keeper = 0; keeper < DUNGEONS_COUNT; keeper++)
    {
        struct Dungeon *dungeon = &state->dungeon[keeper];
        if (dungeon->num_things_in_hand == 0)
            continue;
        for (int user = 0; user < MAX_NET_USERS; user++)
        {
            struct UserState *user_state = &state->user_states[user];
            if (user_state->player_id != keeper)
                continue;
            if (user_state->num_things_in_hand == 0)
            {
                memcpy(user_state->things_in_hand, dungeon->things_in_hand, sizeof(user_state->things_in_hand));
                user_state->num_things_in_hand = std::min<int>(dungeon->num_things_in_hand, MAX_THINGS_IN_HAND);
                memset(dungeon->things_in_hand, 0, sizeof(dungeon->things_in_hand));
                dungeon->num_things_in_hand = 0;
            }
            break;
        }
    }
    return 0;
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static const struct SaveMigration save_migrations[] = {
    { "Game", 9, "the hand moves from the keeper to its user", migrate_hand_to_user },
    { nullptr, 0, nullptr, nullptr }
};

uint32_t save_migration_count(void)
{
    return static_cast<uint32_t>(std::size(save_migrations)) - 1;   // the last entry only keeps the table from being empty
}

const struct SaveMigration *save_migration_get(uint32_t index)
{
    // cppcheck-suppress unsignedLessThanZero
    return (index < save_migration_count()) ? &save_migrations[index] : nullptr;
}

enum SaveResult save_migrations_run_table(struct SaveLoadCtx *context, const char *struct_name,
    const struct SaveMigration *table, uint32_t count, struct SaveError *err)
{
    // The migrations of this struct that the file's minor still needs, oldest first.
    std::vector<const SaveMigration *> todo;
    for (uint32_t index = 0; index < count; index++)
    {
        if ((strcmp(table[index].struct_name, struct_name) == 0) && (table[index].from_minor >= context->file_minor))
            todo.push_back(&table[index]);
    }
    std::stable_sort(todo.begin(), todo.end(),
        [](const SaveMigration *left, const SaveMigration *right) { return left->from_minor < right->from_minor; });
    const auto failed = std::find_if(todo.begin(), todo.end(), [context](const SaveMigration *migration) {
        return migration->migrate(context) != 0;
    });
    if (failed != todo.end())
        return save_fail(err, SVR_Unsupported, "migration %s failed", (*failed)->name);
    return SVR_Ok;
}

enum SaveResult save_migrations_run(struct SaveLoadCtx *context, const char *struct_name, struct SaveError *err)
{
    return save_migrations_run_table(context, struct_name, save_migrations, save_migration_count(), err);
}
