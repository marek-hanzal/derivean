import {type IContainer}        from "@use-pico/container";
import {withRepositoryHandler}  from "@use-pico/rpc-server";
import {ItemRepository}         from "../repository/ItemRepository";
import {ItemTypeRepository}     from "../repository/ItemTypeRepository";
import {ItemRpc}                from "../rpc/ItemRpc";
import {ItemTypeRpc}            from "../rpc/ItemTypeRpc";
import {withItemRepository}     from "./withItemRepository";
import {withItemTypeRepository} from "./withResourcTypeeRepository";

export const withItemContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     ItemRepository,
        withRepository: withItemRepository,
        handler:        ItemRpc,
    });
    withRepositoryHandler({
        container,
        repository:     ItemTypeRepository,
        withRepository: withItemTypeRepository,
        handler:        ItemTypeRpc,
    });
};
