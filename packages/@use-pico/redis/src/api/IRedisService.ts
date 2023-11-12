import {type Redis} from "./Redis";

export interface IRedisService {
    readonly redis: Redis;

    cache<TValue>(key: string | any[], value: () => Promise<TValue>): Promise<TValue>;
}
