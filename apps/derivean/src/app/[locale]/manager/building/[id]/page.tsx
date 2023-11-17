import {
    BuildingPreview,
    withBuildingRepository
}                     from "@derivean/building";
import {BuildingMenu} from "@derivean/manager";
import {container}    from "@derivean/server";
import {BuildingIcon} from "@derivean/ui";
import {
    Grid,
    GridCol,
    HomeIcon,
    ListIcon,
    Nav,
    Page
}                     from "@use-pico/client";
import {t}            from "@use-pico/translator";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const building = await withBuildingRepository.use(container).getOrThrow(id);

    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: t({values: building})`Building detail`,
        }}
        postfix={<Nav
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/building/list",
                    label: t()`Building list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        append={<BuildingMenu
            buildingId={building.id}
            active={["/manager/building/[id]"]}
        />}
    >
        <Grid pb={"md"}>
            <GridCol span={6}>
                <BuildingPreview
                    building={building}
                />
            </GridCol>
        </Grid>
    </Page>;
}
