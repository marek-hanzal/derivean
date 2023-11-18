import {
    type Connection,
    withService
}                      from "@use-pico/server";
import {type Database} from "../api/Database";

export const withConnection = withService<Connection<Database>>("@use-pico/server/Connection");
export type withConnection = typeof withConnection["service"];
