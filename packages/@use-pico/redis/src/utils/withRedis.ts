import {Redis} from "ioredis";

export namespace withRedis {
    export interface Props {
        url?: string;
    }
}

export const withRedis = (
    {
        url = process.env.REDIS_URL,
    }: withRedis.Props
) => {
    if (!url) {
        throw new Error(`No env.REDIS_URL provided!`);
    }
    return new Redis(url);
};
