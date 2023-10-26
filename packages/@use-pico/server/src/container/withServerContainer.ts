import {withAuthContainer} from "@use-pico/auth";
import {
    Container,
    type IContainer
}                          from "@use-pico/container";
import {withOrmContainer}  from "@use-pico/orm";

export const withServerContainer = (instance?: IContainer.Instance) => {
    const container = new Container(instance);

    const register = [
        withAuthContainer,
        withOrmContainer,
    ] as const;

    register.map(register => register(container));

    return container;
};
