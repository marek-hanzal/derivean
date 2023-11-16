import {type IContainer}       from "@use-pico/container";
import {withRepositoryHandler} from "@use-pico/rpc-server";
import {HeroRepository}        from "../repository/HeroRepository";
import {HeroRpc}               from "../rpc/HeroRpc";
import {withHeroRepository}    from "./withHeroRepository";

export const withHeroContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     HeroRepository,
        withRepository: withHeroRepository,
        handler:        HeroRpc,
    });
};
