import {type IContainer}        from "@use-pico/container";
import {withRepositoryHandler}  from "@use-pico/rpc-server";
import {withBuildingMutation}   from "../mutation/withBuildingMutation";
import {withBuildingCountQuery} from "../query/withBuildingCountQuery";
import {withBuildingQuery}      from "../query/withBuildingQuery";
import {BuildingRepository}     from "../repository/BuildingRepository";
import {withBuildingRepository} from "./withBuildingRepository";

export const withBuildingContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     BuildingRepository,
        withRepository: withBuildingRepository,
        handler:        {
            query:    withBuildingQuery,
            count:    withBuildingCountQuery,
            mutation: withBuildingMutation,
        },
    });
};
