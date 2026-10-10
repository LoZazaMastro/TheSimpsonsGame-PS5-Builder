# The game's Lua scripting API

The Simpsons Game drives a surprising amount of itself from plain-text Lua that ships in every
install: `gamedata/simpsons_gameflow.lua`, `simpsons_gameflow_helpers.lua`, and
`simpsons_scores.lua`. This file catalogs the scripting vocabulary those files use, so you can
see what the game lets you express without touching a single binary format.

Two important caveats up front. First, everything here was read off the game's own scripts, not
off the native implementation. The call signatures are exact (they're right there in the Lua),
but the precise behavior of each native method is inferred from its name and how the shipped
scripts use it. Second, some of these are Lua helpers defined in the script files themselves
(fully readable, you can go look), and some are native functions bound through tolua (the C++
side). Where it matters, it's noted. To confirm or extend the native ones, the tolua bindings
register their method names as strings in the executable, so they're discoverable the same way
the cutscene investigation found `MoviePkg` (see `cutscene_skip.md`).

Counts below are how many times the shipped scripts call each thing, as a rough guide to how
load-bearing it is.

## Game structure

The campaign is a tree: a Game holds Episodes, an Episode holds Modes, a Mode holds an ordered
list of entries (Maps, Movies, MetricScreens). These builders are Lua helpers in
`simpsons_gameflow_helpers.lua`, so you can read exactly what they do.

    NewGame( name, type )
    NewEpisode( game, name, localizedKey, completeScore, lockedScore, cheatScoreEvent, richPresenceID )
    NewMode( episode, modeType, modeName, completionEvent, numPlayers, startEvent, restartEvent, updateTimeEvent )
    NewMap( mapName, folder, stream, checkpoint, completionEvent )
    NewMovie( movieName, stream, movieType, localizedKey, movieVolume )
    NewMetricScreen( name, bindingName, titleKey, requiredScore, requiredValue )

Observed values: `modeType` is `"MODE_STANDARD"` or `"MODE_TIMED"`. `movieType` is `"INTRO"` or
`"OUTRO"`. `NewMap`'s `stream` is a `.str` filename (e.g. `"loc.str"`) and `folder` is the level
folder it lives in (e.g. `"loc"`). `movieVolume` is 0.0 to 1.0.

A real mission, assembled entirely in script (Land of Chocolate, from the shipped file):

    episode = NewEpisode( game, "LAND_OF_CHOCOLATE", "FE_Episode_loc",
                          "SCORE_LAND_OF_CHOCOLATE_COMPLETE", "SCORE_LAND_OF_CHOCOLATE_LOCKED",
                          "SCORE_EVENT_LAND_OF_CHOCOLATE_CHEAT", CONTEXT_EPISODE_CHOCOLATE )
    episode:SetDefault()
        mode = NewMode( episode, "MODE_STANDARD", "FE_GameMode_Standard", nil, "GameMode_OnePlayer",
                        "SCORE_EVENT_LAND_OF_CHOCOLATE_START", ... )
            mode:AddEntry( NewMovie( "Homer's Dream", "loc_igc01", "INTRO", "FE_FMV_01", 0.85 ) )
            map = NewMap( "loc map", "loc", "loc.str", nil, "SCORE_EVENT_LAND_OF_CHOCOLATE_COMPLETE" )
            map:SetNextEpisode( "SPR_HUB" )
            mode:AddEntry( map )
            mode:AddEntry( NewMovie( "Homer Explodes", "loc_igc02", "OUTRO", "FE_FMV_02", 0.85 ) )
            mode:AddEntry( NewMetricScreen( "Completed Land of Chocolate", "COMPLETED_LAND_OF_CHOCOLATE", "" ) )

That is the entire definition of a story mission: an intro movie, a playable map, an outro movie,
a results screen, wired to a completion score and a next-episode link.

## Map methods

    mode:AddEntry( entry )                      -- add a map/movie/screen to a mode's sequence   (119)
    map:SetNextEpisode( episodeName )           -- where this map leads when finished             (3)
    map:SetPlayerOverride( slot, modelRef )     -- force a character model for a player slot       (5)
    map:MarkAsExtension()                       -- flag this map as an extension/DLC-style add     (2)
    map:SetCompletionScoreEvent( event )
    object:SetCompletionScoreEvent( event )     -- also on other entry objects                     (2)

`SetPlayerOverride` is worth calling out. It picks which character model loads for a player,
by a reference string like `"{E9B13167-...}:lisa_sx2_hog3_h2"` (a GUID plus a model name). So
*which* model a mission uses is scriptable today, even though authoring a *new* model still needs
the `.str` format cracked (see `str_format.md`). You can reassign existing models freely.

## Episode methods

    episode:SetDefault()                        -- mark as the default episode
    episode:SetRoot()                           -- (2)
    episode:SetReplayDisabled()                 -- (2)
    episode:DisableQuit()                       -- (2)
    episode:SetBusStopFail()

## Scoring, objectives, and unlocks

This is the game's objective and progression engine, and it's enormous: 136 score definitions,
72 score events, all in script. `sk` is the ScoreKeeper. A "score" is any tracked value (a
counter, a flag, a lock state); a "score event" is a named action that modifies scores; a
"watcher" fires when a score changes in some way.

    sk = ScoreKeeper:GetScoreKeeper()
    sk:SetScoringVersion( n )
    sk:BeginScoreInstallation()  ...  sk:FinalizeScoreInstallation()   -- wrap your score setup
    sk:CreateScore( "SCORE_NAME", initialValue )                        -- (136)
    sk:CreateScoreEvent( "SCORE_EVENT_NAME" )                           -- (72)
    sk:FindScore( "SCORE_NAME" )                                        -- (65)
    sk:FindEvent( "SCORE_EVENT_NAME" )                                  -- (4)
    sk:SetScore( score, value )                                         -- (2)

Score objects:

    score:SetStorage( ... )                     -- (116)   how/where the value persists
    score:AddDifferenceWatcher( ... )           -- (56)    fire when the value changes by some delta
    score:AddUnlockWatcher( ... )               -- (44)    fire to unlock something
    score:SetConstraint( ... )                  -- (31)
    score:SetBounds( min, max )                 -- (15)
    score:AddComparisonWatcher( ... )           -- (11)
    score:AddMessageWatcher( ... )              -- (5)     fire and show a message
    score:AddEventWatcher( ... )                -- (1)
    score:MarkAsStatistic()                     -- (2)     track as a stat, not a gameplay value

Score events carry ops that run when the event fires:

    event:AddNumericOp( "set"|"add"|"subtract", "SCORE_NAME", value )   -- (123)
    event:AddTimeOp( ... )                                              -- (9)
    event:AddMessageOp( "iMsgSomething" )                              -- (6)   show a message
    event:AddEventOp( ... )                                             -- (2)   chain another event

A real objective (Bart/Homer power upgrade, from the shipped file):

    event = sk:CreateScoreEvent("SCORE_EVENT_POWER_INCREASE")
    event:AddNumericOp("set", "SCORE_TROPHIES_NEEDED_BEFORE_POWER_UPGRADE", numTrophiesRequired )
    event:AddNumericOp("add", "SCORE_POWER_MAX_BART", incVal)
    event:AddMessageOp("iMsgBARTPowerIncreased")

### Unlock gating (level progression)

The "beat this to unlock that" logic that gates the whole campaign is built from small boolean
gates, and they are Lua helpers defined right in `simpsons_scores.lua` (readable, editable):

    CreateNotGate( inputScore, "OUTPUT_LOCKED_SCORE" )
    CreateAndGate( inputScore, "OUTPUT_LOCKED_SCORE" )
    CreateOrGate( inputScore, "OUTPUT_LOCKED_SCORE" )
    CreateNandGate( { score1, score2, ... }, "OUTPUT_LOCKED_SCORE" )

For example, unlocking Chapter 3 once Bargain Bin is done:

    CreateNotGate( sk:FindScore("SCORE_BARGAIN_BIN_COMPLETE"), "SCORE_GAME_HUB_LOCKED" )

Edit these and you rewire the campaign: unlock everything from the start, change the order, add
gates for a new episode. It's all here in text.

## Costumes

    costumeRegistry = CostumeRegistry:GetCostumeRegistry()
    costumeRegistry:RegisterCostume( ... )      -- (12)
    Registry:RegisterCostume( ... )             -- (12, alternate handle)

## Managers

    GameFlowManager:GetGameFlowManager()
    FlowManager:SetGame( game )
    FlowManager:SetGameFlowVersion( n )
    ScoreKeeper:SetScoringVersion( n )

## What this means for modding

Most of a "new mission" is expressible in this API right now, with no binary format work: a new
Episode and Mode, reusing existing maps and movies, with new objectives, unlock gates, and player
model overrides. The hard wall is only new *content* (a genuinely new map's geometry, a new
model's mesh), which lives inside `.str` files. See `docs/MODDING_ADVANCED.md` for how the pieces
fit and what's blocked on what.
