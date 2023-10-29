import {type FC} from "react";

export namespace FileTableAction {
    export interface Props {
        // withFileSourceQuery: WithSourceQuery<FileSchema, FileQuerySchema>;
        // withFileMutation: WithMutation<FileMutationSchema, FileSchema>;
    }
}

export const FileTableAction: FC<FileTableAction.Props> = (
    {
        // withFileSourceQuery,
        // withFileMutation,
    }
) => {
    return "FileTableAction";

    // const query = withFileSourceQuery.store.use(({filter}) => ({filter}));
    // const fileMutation = withFileMutation.useMutation();
    // const successNotification = useSuccessNotification();
    // const errorNotification = useErrorNotification();
    //
    // return <>
    //     <TableActionMenu>
    //         <MenuItem
    //             leftSection={<IconWashDrycleanOff/>}
    //             withLabel={"stale.label"}
    //             color={"orange"}
    //             disabled={!query.filter || fileMutation.isPending}
    //             onClick={() => {
    //                 query.filter && fileMutation.mutate(
    //                     {
    //                         update: {
    //                             update: {
    //                                 ttl: 0,
    //                             },
    //                             query:  {
    //                                 filter: query.filter,
    //                             },
    //                         },
    //                     },
    //                     {
    //                         onSuccess: () => {
    //                             successNotification({
    //                                 withTranslation: {
    //                                     label: "stale",
    //                                 }
    //                             });
    //                         },
    //                         onError:   () => {
    //                             errorNotification({
    //                                 withTranslation: {
    //                                     label: "stale",
    //                                 }
    //                             });
    //                         },
    //                     }
    //                 );
    //             }}
    //         />
    //     </TableActionMenu>
    // </>;
};
