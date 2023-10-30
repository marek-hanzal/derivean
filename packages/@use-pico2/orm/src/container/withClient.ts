import {withService}   from "@use-pico2/container";
import {type Client}   from "../api/Client";
import {type Database} from "../api/Database";

export const withClient = withService<Client<Database>>("@use-pico2/orm/Client");
export type withClient = typeof withClient;
