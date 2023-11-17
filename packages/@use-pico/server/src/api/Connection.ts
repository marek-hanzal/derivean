import {type Kysely}   from "kysely";
import {type Database} from "./Database";

export type Connection<TDatabase extends Database> = Kysely<TDatabase>;
