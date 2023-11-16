import {type IContainer}       from "@use-pico/container";
import {withRepositoryHandler} from "@use-pico/rpc-server";
import {HeroRepository}        from "../repository/HeroRepository";
import {HeroRpc}               from "../rpc/HeroRpc";
import {HeroService}           from "../service/HeroService";
import {withHeroRepository}    from "./withHeroRepository";
import {withHeroService}       from "./withHeroService";

export const withHeroContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     HeroRepository,
        withRepository: withHeroRepository,
        handler:        HeroRpc,
    });
    withHeroService.bind(container, HeroService);
};
