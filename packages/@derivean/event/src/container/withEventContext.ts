import {type IContainer}       from "@use-pico/container";
import {withRepositoryHandler} from "@use-pico/rpc-server";
import {EventRepository}       from "../repository/EventRepository";
import {EventRpc}              from "../rpc/EventRpc";
import {withEventRepository}   from "./withEventRepository";

export const withEventContext: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     EventRepository,
        withRepository: withEventRepository,
        handler:        EventRpc,
    });
};
