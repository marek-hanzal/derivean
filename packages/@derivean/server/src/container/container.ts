import {withBuildingContainer}  from "@derivean/building";
import {withEventContainer}     from "@derivean/event";
import {withInventoryContainer} from "@derivean/inventory";
import {withItemContainer}      from "@derivean/item";
import {withKingdomContainer}   from "@derivean/kingdom";
import {withConnection}         from "@derivean/orm";
import {withProducerContainer}  from "@derivean/producer";
import {
    createConnection,
    createRedisClient,
    withRedis,
    withServerContainer
}                               from "@use-pico/server";
import {withContext}            from "./withContext";

const register = [
    withBuildingContainer,
    withEventContainer,
    withInventoryContainer,
    withItemContainer,
    withKingdomContainer,
    withProducerContainer,
] as const;

export const container = withServerContainer();

withConnection.factory(container, () => createConnection({}));
withRedis.factory(container, () => createRedisClient({}));

register.forEach(register => register(container));

withContext(container);
