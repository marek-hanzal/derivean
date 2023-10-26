import {
    Container,
    type IContainer
} from "@use-pico/container";

export const withContainer = (instance?: IContainer.Instance) => {
    return new Container(instance);
};
