import {type Database as CoolDatabase} from "@use-pico/orm";
import {type GeneratedAlways}          from "kysely";

export type Database =
    CoolDatabase
    & {
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
             * Inventory connected to this building
             */
            inventoryId: string;
            /**
             * Maximum number of buildings of this type.
             */
            maximum: number;
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
         * Every building could have more producers.
         */
        BuildingProducer: {
            id: GeneratedAlways<string>;
            buildingId: string;
            producerId: string;
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
             * Optional duration of an event; used to compute event instance from/to.
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
         * Implementation of item transaction event.
         *
         * This could be, for example, give-away for a new players or event
         * disaster event which takes some items.
         *
         * Takes inventory of this event and apply it to a kingdom inventory of a player.
         */
        EventItem: {
            id: GeneratedAlways<string>;
            eventId: string;
            inventoryId: string;
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
    }
