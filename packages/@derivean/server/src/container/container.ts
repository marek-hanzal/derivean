import {withBuildingContainer}  from "@derivean/building";
import {withInventoryContainer} from "@derivean/inventory";
import {withItemContainer}      from "@derivean/item";
import {withProducerContainer}  from "@derivean/producer";
import {
    withClient,
    withKysely
}                               from "@use-pico/orm";
import {
    withRedis,
    withRedisClient
}                               from "@use-pico/redis";
import {withServerContainer}    from "@use-pico/server";

const register = [
    withBuildingContainer,
    withProducerContainer,
    withItemContainer,
    withInventoryContainer,
] as const;

export const container = withServerContainer();

withClient.factory(container, () => withKysely({}));
withRedis.factory(container, () => withRedisClient({}));

register.forEach(register => register(container));
