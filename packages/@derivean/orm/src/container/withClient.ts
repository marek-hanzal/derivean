import {withService}   from "@use-pico/container";
import {type Client}   from "@use-pico/orm";
import {type Database} from "../api/Database";

export const withClient = withService<Client<Database>>("@use-pico/orm/Client");
export type withClient = typeof withClient;
