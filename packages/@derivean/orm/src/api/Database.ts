import {type Database as CoolDatabase} from "@use-pico2/orm";
import {type GeneratedAlways}          from "kysely/dist/esm";

export interface Database extends CoolDatabase {
    Resource: {
        id: GeneratedAlways<string>;
        name: string;
    };
}
