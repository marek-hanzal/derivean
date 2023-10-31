import {type Database as CoolDatabase} from "@use-pico/orm";
import {type GeneratedAlways}          from "kysely";

export type Database =
    CoolDatabase
    & {
        Resource: {
            id: GeneratedAlways<string>;
            name: string;
        };
    }
