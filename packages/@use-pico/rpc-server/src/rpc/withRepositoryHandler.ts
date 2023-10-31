import {
    type IContainer,
    withService
}                            from "@use-pico/container";
import {
    type CountSchema,
    type IWithMutation,
    type IWithQuery,
    type QuerySchema
}                            from "@use-pico/query";
import {type IRepository}    from "@use-pico/repository";
import {type ArraySchema}    from "@use-pico/schema";
import {type MutationSchema} from "@use-pico/source";

export namespace withRepositoryHandler {
    export interface Props<
        TSchema extends IRepository.Schema<any, any, QuerySchema<any, any>, MutationSchema<any, any>>,
        TWithRepository extends IRepository<any, TSchema, any>,
        TRepository extends new (...args: any) => TWithRepository,
        TQueryHandler extends IWithQuery<TSchema["query"], ArraySchema<TSchema["entity"]>>,
        TCountHandler extends IWithQuery<TSchema["query"], CountSchema>,
        TMutationHandler extends IWithMutation<TSchema["mutation"], TSchema["entity"]>,
    > {
        container: IContainer.Type;
        withRepository: withService.Service<TWithRepository>;
        repository: TRepository;
        handler: {
            query: TQueryHandler;
            count: TCountHandler;
            mutation: TMutationHandler;
        };
    }
}

export const withRepositoryHandler = <
    TSchema extends IRepository.Schema<any, any, any, any>,
    TWithRepository extends IRepository<any, TSchema, any>,
    TRepository extends new (...args: any) => TWithRepository,
    TQueryHandler extends IWithQuery<TSchema["query"], ArraySchema<TSchema["entity"]>>,
    TCountHandler extends IWithQuery<TSchema["query"], CountSchema>,
    TMutationHandler extends IWithMutation<TSchema["mutation"], TSchema["entity"]>,
>(
    {
        container,
        withRepository,
        repository,
    }: withRepositoryHandler.Props<
        TSchema,
        TWithRepository,
        TRepository,
        TQueryHandler,
        TCountHandler,
        TMutationHandler
    >
) => {
    withRepository.bind(container, repository);


};
