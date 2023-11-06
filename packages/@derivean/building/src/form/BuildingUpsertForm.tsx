import {BuildingIcon}         from "@derivean/ui";
import {
    Form,
    TextInput
}                             from "@use-pico/form";
import {type WithEntity}      from "@use-pico/types";
import {type FC}              from "react";
import {withBuildingMutation} from "../mutation/withBuildingMutation";
import {BuildingSchema}       from "../schema/BuildingSchema";
import {BuildingShapeSchema}  from "../schema/BuildingShapeSchema";

export namespace BuildingUpsertForm {
    export type Props =
        Form.PropsEx<
            withBuildingMutation,
            BuildingShapeSchema,
            withBuildingMutation["schema"]["request"],
            withBuildingMutation["schema"]["response"]
        >
        & WithEntity.Schema.$<BuildingSchema>;
}

export const BuildingUpsertForm: FC<BuildingUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        icon={<BuildingIcon/>}
        withMutation={withBuildingMutation}
        schema={BuildingShapeSchema}
        inputs={{
            name: TextInput,
        }}
        values={entity}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            name: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
        </>}
        {...props}
    />;
};
