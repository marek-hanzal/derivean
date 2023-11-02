import {withProducerContainer} from "@derivean/producer";
import {withResourceContainer} from "@derivean/resource";
import {
    withClient,
    withKysely
}                              from "@use-pico/orm";
import {withServerContainer}   from "@use-pico/server";

const register = [
    withProducerContainer,
    withResourceContainer,
] as const;

export const container = withServerContainer();
withClient.factory(container, () => withKysely({}));

register.forEach(register => register(container));
