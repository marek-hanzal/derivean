import {Redis} from "ioredis";

export namespace createRedisClient {
    export interface Props {
        url?: string;
    }
}

export const createRedisClient = (
    {
        url = process.env.REDIS_URL,
    }: createRedisClient.Props
) => {
    if (!url) {
        throw new Error(`No env.REDIS_URL provided!`);
    }
    return new Redis(url, {
        lazyConnect: true,
    });
};
