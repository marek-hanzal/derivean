import type Redis    from "ioredis";
import {withService} from "../service/withService";

export const withRedis = withService<Redis>("@use-pico/server/Redis");
export type withRedis = typeof withRedis;
