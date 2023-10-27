import {withAuthContainer} from "@use-pico/auth";
import {
    Container,
    type IContainer
}                          from "@use-pico/container";

export const withServerContainer = (instance?: IContainer.Instance) => {
    const container = new Container(instance);

    const register = [
        withAuthContainer,
    ] as const;

    register.forEach(register => register(container));

    return container;
};
