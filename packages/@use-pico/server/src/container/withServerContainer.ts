import {type IContainer}          from "../api/IContainer";
import {Container}                from "./Container";
import {withAuthContainer}        from "./withAuthContainer";
import {withCacheContainer}       from "./withCacheContainer";
import {withContainer}            from "./withContainer";
import {withRpcContainer}         from "./withRpcContainer";
import {withTranslationContainer} from "./withTranslationContainer";

export const withServerContainer = (instance?: IContainer.Instance) => {
    const container = new Container(instance);
    withContainer.value(container, container);

    const register = [
        withAuthContainer,
        withCacheContainer,
        withRpcContainer,
        withTranslationContainer,
    ] as const;

    register.forEach(register => register(container));

    return container;
};
