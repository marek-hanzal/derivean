import {type Database as CoolDatabase} from "@use-pico/orm";
import {type GeneratedAlways}          from "kysely";

export type Database =
    CoolDatabase
    & {
        /**
         * Definition of a building. This is a template for building instances.
         * Every user's building instance is created from this template.
         */
        Building: {
            id: GeneratedAlways<string>;
            /**
             * Building name, goes through translations
             */
            name: string;
            /**
             * Construction time in seconds
             */
            construction: number;
            /**
             * Maximum number of buildings of this type.
             */
            maximum: number;
        };
        /**
         * Defines what's needed to build a building.
         *
         * Given items are subtracted from the kingdom's inventory.
         */
        BuildingConstructionRequirement: {
            id: GeneratedAlways<string>;
            buildingId: string;
            itemId: string;
            amount: number;
        };
        /**
         * When a player builds a building, it's represented by this instance.
         */
        BuildingInstance: {
            id: GeneratedAlways<string>;
            buildingId: string;
            kingdomId: string;
            /**
             * Instance's inventory is copied from the building's inventory.
             */
            inventoryId: string;
            userId: string;
            created: string;
            level: number;
        };
        /**
         * Buildings can have perks too!
         */
        BuildingPerk: {
            id: GeneratedAlways<string>;
            buildingId: string;
            perkId: string;
        };
        /**
         * Every building could have more producers.
         */
        BuildingProducer: {
            id: GeneratedAlways<string>;
            buildingId: string;
            producerId: string;
        };
        /**
         * General building requirements that need to be met to enable
         * building construction.
         *
         * Those items are not subtracted from the kingdom's inventory.
         */
        BuildingRequirement: {
            id: GeneratedAlways<string>;
            buildingId: string;
            itemId: string;
            amount: number;
        };
        /**
         * General event definition
         */
        Event: {
            id: GeneratedAlways<string>;
            name: string;
            type: string;
            /**
             * Time boundary when an event should automatically trigger (create an instance or instantly apply an effect)
             */
            from?: string;
            /**
             * Time boundary when an event should automatically trigger (create an instance or instantly apply an effect)
             */
            to?: string;
            /**
             * Optional duration (in days) of an event; used to compute event instance from/to.
             */
            duration?: number;
            /**
             * Instant events are executed immediately when a player does an action or
             * when a player logs-in.
             */
            instant?: boolean;
            /**
             * User-specific event
             */
            userId?: string;
        };
        /**
         * Event that generates heroes (based on an amount).
         */
        EventHero: {
            id: GeneratedAlways<string>;
            eventId: string;
            amount: number;
        };
        /**
         * Event instance is bound to a player and kingdom.
         *
         * Player must commit instance to get the output.
         */
        EventInstance: {
            id: GeneratedAlways<string>;
            eventId: string;
            kingdomId: string;
            userId: string;
            from?: string;
            to?: string;
            /**
             * Used to properly show UI - when an event is instant, user just confirms it.
             */
            instant?: boolean;
            /**
             * Committed events are already applied.
             */
            commit?: boolean;
        };
        /**
         * Implementation of item transaction event.
         *
         * This could be, for example, give-away for a new players or event
         * disaster event which takes some items.
         *
         * Takes inventory of this event and apply it to a kingdom inventory of a player.
         */
        EventInventory: {
            id: GeneratedAlways<string>;
            eventId: string;
            inventoryId: string;
        };
        /**
         * Here is general "character" used to do anything.
         *
         * It can be used in buildings, to fight, to basically do all the things in the game.
         */
        Hero: {
            id: GeneratedAlways<string>;
            /**
             * User who owns this hero
             */
            userId: string;
            /**
             * Where this hero is located
             */
            kingdomId: string;
            /**
             * Yaaay, hero's name!
             */
            name: string;
            /**
             * Current hero's health; perks define maximum.
             */
            health: number;
            /**
             * Current level of the hero
             */
            level: number;
            /**
             * Prestige is kind of hero's "rarity"
             */
            prestige: number;
        };
        /**
         * Perks assigned to the given hero
         */
        HeroPerk: {
            id: GeneratedAlways<string>;
            heroId: string;
            perkId: string;
        };
        /**
         * Every item available in the game used for all mechanics.
         */
        Item: {
            id: GeneratedAlways<string>;
            name: string;
            typeId: string;
        };
        /**
         * Organize items in categories: for example, item, weapon, item, ...
         */
        ItemType: {
            id: GeneratedAlways<string>;
            name: string;
        };
        /**
         * Kingdom collects all the building and items a user have.
         *
         * User can have more kingdoms.
         */
        Kingdom: {
            id: GeneratedAlways<string>;
            /**
             * Overall kingdom's inventory
             */
            inventoryId: string;
            userId: string;
            created: string;
            name: string;
        };
        /**
         * Perks are used to define hero's abilities. All the computations in the game are based on perks.
         *
         * "Perk" is maybe a bit incorrect word, but it's much shorter, than "Attribute".
         */
        Perk: {
            id: GeneratedAlways<string>;
            /**
             * Perk's name
             */
            name: string;
            /**
             * Perk modifier value
             */
            value: number;
            /**
             * Perk's level
             */
            level: number;
            /**
             * Before computation is done, perks are ordered by this value.
             */
            order: number;
        };
        /**
         * Template for producers: defines what's an input and
         * what's going out during specified time.
         */
        Producer: {
            id: GeneratedAlways<string>;
            name: string;
            time: number;
        };
        /**
         * Input for a producer; defines consumed item of the given
         * amount and time.
         */
        ProducerInput: {
            id: GeneratedAlways<string>;
            producerId: string;
            itemId: string;
            amount: number;
        };
        /**
         * Output of a producer; defines produced item of the given
         * amount and time.
         */
        ProducerOutput: {
            id: GeneratedAlways<string>;
            producerId: string;
            itemId: string;
            amount: number;
        };
    }
