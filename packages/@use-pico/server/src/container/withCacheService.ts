import {type ICacheService} from "../api/ICacheService";
import {withService}        from "../service/withService";

export const withCacheService = withService<ICacheService>("@use-pico/server/CacheService");
export type withCacheService = typeof withCacheService;
