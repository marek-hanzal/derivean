import {withBuildingContainer}  from "@derivean/building";
import {withInventoryContainer} from "@derivean/inventory";
import {withProducerContainer}  from "@derivean/producer";
import {withResourceContainer}  from "@derivean/resource";
import {
    withClient,
    withKysely
}                               from "@use-pico/orm";
import {
    withRedis,
    withRedisService
}                               from "@use-pico/redis";
import {withServerContainer}    from "@use-pico/server";

const register = [
    withBuildingContainer,
    withProducerContainer,
    withResourceContainer,
    withInventoryContainer,
] as const;

export const container = withServerContainer();
withClient.factory(container, () => withKysely({}));
withRedisService.factory(container, () => withRedis({}));

register.forEach(register => register(container));
