import {withService} from "@use-pico/container";
import {type Redis}  from "../api/Redis";

export const withRedisService = withService<Redis>("@use-pico/redis/Redis");
export type withRedisService = typeof withRedisService;
