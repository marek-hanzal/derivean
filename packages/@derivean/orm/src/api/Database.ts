import {type Database as CoolDatabase} from "@use-pico/orm";
import {type GeneratedAlways}          from "kysely";

export type Database =
    CoolDatabase
    & {
        Resource: {
            id: GeneratedAlways<string>;
            name: string;
        };
        ProducerInput: {
            id: GeneratedAlways<string>;
            producerId: string;
            resourceId: string;
            amount: number;
        };
        ProducerOutput: {
            id: GeneratedAlways<string>;
            producerId: string;
            resourceId: string;
            amount: number;
        }
    }
