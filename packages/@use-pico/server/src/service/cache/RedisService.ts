import {isString}           from "@use-pico/utils";
import type Redis           from "ioredis";
import {type ICacheService} from "../../api/ICacheService";
import {withRedis}          from "../../container/service/withRedis";
import {lazyOf}             from "../../utils/lazyOf";

export class RedisService implements ICacheService {
    static inject = [
        lazyOf(withRedis.inject),
    ];

    constructor(
        protected redis: Redis,
    ) {
    }

    public async cache<TValue>(key: string | any[], value: () => Promise<TValue>, bypass: boolean = false): Promise<TValue> {
        if (bypass) {
            return value();
        }
        const $key = isString(key) ? key : JSON.stringify(key);
        const cached = await this.redis.get($key);
        if (cached) {
            return JSON.parse(cached) as TValue;
        }
        const $value = await value();
        this.redis.set($key, JSON.stringify($value));
        return $value;
    }

    public async clear(): Promise<void> {
        await this.redis.flushall();
    }
}
