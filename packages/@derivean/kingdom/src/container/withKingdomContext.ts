import {type IContainer}       from "@use-pico/container";
import {withRepositoryHandler} from "@use-pico/rpc-server";
import {KingdomRepository}     from "../repository/KingdomRepository";
import {KingdomRpc}            from "../rpc/KingdomRpc";
import {withKingdomRepository} from "./withKingdomRepository";

export const withKingdomContext: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     KingdomRepository,
        withRepository: withKingdomRepository,
        handler:        KingdomRpc,
    });
};
