import {withBuildingContainer}  from "@derivean/building";
import {withInventoryContainer} from "@derivean/inventory";
import {withProducerContainer}  from "@derivean/producer";
import {withResourceContainer}  from "@derivean/resource";
import {
    withClient,
    withKysely
}                               from "@use-pico/orm";
import {withServerContainer}    from "@use-pico/server";

const register = [
    withBuildingContainer,
    withProducerContainer,
    withResourceContainer,
    withInventoryContainer,
] as const;

export const container = withServerContainer();
withClient.factory(container, () => withKysely({}));

register.forEach(register => register(container));
